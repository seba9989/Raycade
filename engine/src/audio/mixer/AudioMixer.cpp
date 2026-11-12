/**
 * @file AudioMixer.cpp
 * @brief Mikser silnika: urządzenie, pula głosów, dźwięki i szyny.
 */

#include "AudioMixer.hpp"

#include <algorithm>
#include <filesystem>
#include <utility>

namespace {

/** @brief Rozmiar puli głosów (największy limit szyny głównej). */
constexpr std::size_t kSlots = 128;

/** @brief Domyślny limit głosów szyny głównej. */
constexpr int kDefaultVoices = 32;

/** @brief Najgłębsze drzewo szyn (ochrona przed cyklem przy liczeniu stanu). */
constexpr int kMaxBusDepth = 64;

/**
 * @brief Zamienia panoramę silnika (-1 lewo, 1 prawo) na wartość raylib.
 * @details raylib 6 używa tej samej skali, a raylib 5.x - od 1 (lewo) przez 0.5 do 0 (prawo).
 */
float RaylibPan(float pan) {
#if RAYLIB_VERSION_MAJOR >= 6
    return pan;
#else
    return 0.5f - 0.5f * pan;
#endif
}

/** @brief Losuje liczbę z przedziału [-1, 1] generatorem raylib (Engine::RandomSeed()). */
float RandomUnit() {
    constexpr int kSteps = 10000;
    return static_cast<float>(::GetRandomValue(-kSteps, kSteps)) / kSteps;
}

} // namespace

AudioMixer::AudioMixer(LogFunction log, PathFunction path)
    : log_(std::move(log)), path_(std::move(path)), slots_(kSlots) {
    buses_.try_emplace("", *this, "").first->second.maxVoices_ = kDefaultVoices;
    current_ = this;
}

AudioMixer::~AudioMixer() {
    Close();
    if (current_ == this) current_ = nullptr;
}

void AudioMixer::Open() {
    if (ready_) return;
    ::InitAudioDevice();
    ready_ = ::IsAudioDeviceReady();
    if (!ready_) log_(LogLevel::Warning, "nie udalo sie otworzyc urzadzenia audio - bez dzwieku");
}

void AudioMixer::Close() {
    for (Slot& slot : slots_)
        if (slot.used) Release(slot, false);
    finished_.clear();
    for (auto& [path, clip] : clips_)
        ::UnloadSound(clip.sound);
    clips_.clear();
    bytes_.clear();
    failed_.clear();
    if (ready_) ::CloseAudioDevice();
    ready_ = false;
}

Audio& AudioMixer::Sound(const std::string& name) {
    return sounds_.try_emplace(name, *this, name).first->second;
}

AudioBus& AudioMixer::Bus(const std::string& name) {
    auto [it, created] = buses_.try_emplace(name, *this, name);
    if (created) log_(LogLevel::Debug, "nowa szyna audio \"" + name + "\"");
    return it->second;
}

Voice AudioMixer::Play(::Audio& audio) {
    if (!ready_) {
        if (!warnedNoDevice_)
            log_(LogLevel::Warning, "Play() bez urzadzenia audio - dzwieki sa wylaczone");
        warnedNoDevice_ = true;
        return {};
    }
    const double now = ::GetTime();
    if (audio.MinInterval() > 0.0f && now - audio.lastPlay_ < audio.MinInterval()) return {};
    audio.lastPlay_ = now;
    const std::string& bus = audio.Bus();
    if (Bus(bus).IsExclusive()) {
        for (std::size_t i = 0; i < slots_.size(); ++i) {
            Slot& other = slots_[i];
            if (!other.used || other.bus != bus || other.stopping) continue;
            // ten sam dźwięk już gra na tej szynie - muzyka nie zaczyna się od nowa
            if (other.audio == &audio) return Handle(i);
            // zgłoszony w tej klatce inny dźwięk - wygrywa ostatnie Play()
            if (!other.started) Release(other, false);
        }
    }
    MakeRoom(audio);
    auto free = std::ranges::find_if(slots_, [](const Slot& slot) { return !slot.used; });
    if (free == slots_.end()) return {}; // nie zdarza się: MakeRoom() zwalnia miejsce
    Slot& slot = *free;
    const std::uint32_t generation = slot.generation;
    slot = Slot{};
    slot.generation = generation;
    slot.used = true;
    slot.audio = &audio;
    slot.bus = bus;
    slot.path = PickFile(audio);
    slot.variation = 1.0f + audio.PitchVariation() * RandomUnit();
    slot.order = ++order_;
    // muzyka (szyna Exclusive()) gra dalej po przeładowaniu sceny, która ją włączyła
    slot.owner = Bus(bus).IsExclusive() ? nullptr : owner_;
    return Handle(static_cast<std::size_t>(free - slots_.begin()));
}

bool AudioMixer::StealOldest(const std::function<bool(const Slot&)>& match) {
    Slot* oldest = nullptr;
    for (Slot& slot : slots_)
        if (slot.used && match(slot) && (!oldest || slot.order < oldest->order)) oldest = &slot;
    if (!oldest) return false;
    Release(*oldest);
    return true;
}

/**
 * @details Kolejno: limit dźwięku, limity szyny dźwięku i jej rodziców (aż do szyny głównej,
 * której limit to rozmiar puli) i na końcu sama pula. W każdym przypadku ustępuje najstarszy
 * głos z danej grupy - nowy dźwięk jest zwykle ważniejszy niż ten, który gra od dawna.
 */
void AudioMixer::MakeRoom(const ::Audio& audio) {
    if (audio.MaxVoices() > 0)
        while (VoiceCount(audio) >= audio.MaxVoices() &&
               StealOldest([&audio](const Slot& s) { return s.audio == &audio; })) {}
    std::string bus = audio.Bus();
    for (int depth = 0; depth < kMaxBusDepth; ++depth) {
        const ::AudioBus& node = Bus(bus);
        const auto within = [this, &bus](const Slot& s) { return IsWithin(s.bus, bus); };
        if (node.MaxVoices() > 0)
            while (std::ranges::count_if(slots_, [&](const Slot& s) {
                       return s.used && within(s);
                   }) >= node.MaxVoices() &&
                   StealOldest(within)) {}
        if (bus.empty()) break;
        bus = node.Parent();
    }
    if (std::ranges::none_of(slots_, [](const Slot& s) { return !s.used; }))
        StealOldest([](const Slot&) { return true; });
}

void AudioMixer::StopOwner(const void* scene) {
    if (!scene) return;
    for (Slot& slot : slots_)
        if (slot.used && slot.owner == scene) Release(slot, false);
    if (listenerScene_ == scene) listener_ = listenerScene_ = nullptr;
}

void AudioMixer::Listen(const void* listener, const void* scene, Vector2 position) {
    listener_ = listener;
    listenerScene_ = scene;
    listenerPosition_ = position;
}

void AudioMixer::Unlisten(const void* listener) {
    if (listener_ == listener) listener_ = listenerScene_ = nullptr;
}

std::optional<Vector2> AudioMixer::ListenerIn(const void* scene) const {
    if (!listener_ || listenerScene_ != scene) return std::nullopt;
    return listenerPosition_;
}

int AudioMixer::ClampVoices(const ::AudioBus& bus, int count) const {
    if (!bus.Name().empty()) return std::max(0, count);
    const int pool = static_cast<int>(kSlots);
    return count <= 0 ? pool : std::min(count, pool);
}

void AudioMixer::Seek(Slot& slot, float seconds) {
    seconds = std::max(0.0f, seconds);
    if (!slot.started) {
        slot.seek = seconds; // po starcie, w Update()
        return;
    }
    if (slot.stream) {
        ::SeekMusicStream(slot.music, seconds);
        return;
    }
    if (!warnedSeek_)
        log_(LogLevel::Warning,
             "PlaybackTime(s) dziala tylko dla dzwiekow Streamed() albo Looped() - pominieto");
    warnedSeek_ = true;
}

float AudioMixer::PlaybackTime(const Slot& slot) const {
    if (!slot.started) return 0.0f;
    return slot.stream ? ::GetMusicTimePlayed(slot.music) : slot.time;
}

float AudioMixer::Duration(const Slot& slot) const {
    if (!slot.started) return 0.0f;
    if (slot.stream) return ::GetMusicTimeLength(slot.music);
    const unsigned int rate = slot.sound.stream.sampleRate;
    return rate > 0 ? static_cast<float>(slot.sound.frameCount) / static_cast<float>(rate) : 0.0f;
}

std::string AudioMixer::PickFile(::Audio& audio) {
    const auto& files = audio.files_;
    if (files.empty()) return audio.Name();
    if (files.size() == 1) return files.front();
    // bez powtórzeń: losujemy spośród pozostałych plików i przeskakujemy poprzedni
    const int count = static_cast<int>(files.size());
    int index = ::GetRandomValue(0, audio.lastFile_ >= 0 ? count - 2 : count - 1);
    if (audio.lastFile_ >= 0 && index >= audio.lastFile_) ++index;
    audio.lastFile_ = index;
    return files[static_cast<std::size_t>(index)];
}

Voice AudioMixer::Handle(std::size_t index) const {
    return Voice(static_cast<std::uint32_t>(index), slots_[index].generation);
}

AudioMixer::Slot* AudioMixer::Find(const Voice& voice) {
    if (voice.generation_ == 0 || voice.slot_ >= slots_.size()) return nullptr;
    Slot& slot = slots_[voice.slot_];
    return slot.used && slot.generation == voice.generation_ ? &slot : nullptr;
}

const AudioMixer::Slot* AudioMixer::Find(const Voice& voice) const {
    return const_cast<AudioMixer*>(this)->Find(voice);
}

/**
 * @details Głos, który jeszcze nie ruszył albo stoi na wstrzymanej szynie, jest zwalniany od
 * razu - wyciszanie i tak by nie postępowało. Pozostałe wyciszają się przez FadeOutTime()
 * dźwięku, licząc od bieżącego poziomu.
 */
void AudioMixer::Stop(Slot& slot) {
    if (slot.stopping) return;
    const float fadeOut = slot.audio->FadeOutTime();
    if (!slot.started || slot.paused || fadeOut <= 0.0f || slot.fade <= 0.0f) {
        Release(slot);
        return;
    }
    slot.stopping = true;
    slot.fadeRate = -1.0f / fadeOut;
}

void AudioMixer::Stop(const ::Audio& audio) {
    for (Slot& slot : slots_)
        if (slot.used && slot.audio == &audio) Stop(slot);
}

void AudioMixer::Stop(const ::AudioBus& bus) {
    for (Slot& slot : slots_)
        if (slot.used && IsWithin(slot.bus, bus.Name())) Stop(slot);
}

int AudioMixer::VoiceCount(const ::Audio& audio) const {
    return static_cast<int>(std::ranges::count_if(slots_, [&audio](const Slot& slot) {
        return slot.used && !slot.stopping && slot.audio == &audio;
    }));
}

int AudioMixer::VoiceCount(const ::AudioBus& bus) const {
    return static_cast<int>(std::ranges::count_if(slots_, [this, &bus](const Slot& slot) {
        return slot.used && !slot.stopping && IsWithin(slot.bus, bus.Name());
    }));
}

void AudioMixer::Reparent(::AudioBus& bus, const std::string& parent) {
    if (bus.Name().empty()) {
        log_(LogLevel::Warning, "szyna glowna nie ma rodzica - pominieto Parent()");
        return;
    }
    Bus(parent); // rodzic powstaje, jeśli go nie ma
    if (IsWithin(parent, bus.Name())) {
        log_(LogLevel::Error,
             "szyna \"" + bus.Name() + "\" pod \"" + parent + "\" utworzylaby cykl - pominieto");
        return;
    }
    bus.parent_ = parent;
}

bool AudioMixer::IsWithin(const std::string& bus, const std::string& ancestor) const {
    if (ancestor.empty()) return true; // wszystko jest pod szyną główną
    std::string current = bus;
    for (int depth = 0; depth < kMaxBusDepth && !current.empty(); ++depth) {
        if (current == ancestor) return true;
        const auto it = buses_.find(current);
        if (it == buses_.end()) return false;
        current = it->second.Parent();
    }
    return false;
}

AudioMixer::BusState AudioMixer::StateOf(const std::string& bus) const {
    BusState state;
    std::string current = bus;
    for (int depth = 0; depth < kMaxBusDepth; ++depth) {
        const auto it = buses_.find(current);
        if (it == buses_.end()) break;
        const ::AudioBus& node = it->second;
        state.gain *= node.IsMuted() ? 0.0f : node.Volume();
        state.paused = state.paused || node.IsPaused();
        if (current.empty()) break; // szyna główna - koniec drzewa
        current = node.Parent();
    }
    return state;
}

void AudioMixer::Failed(const std::string& path) {
    if (!failed_.insert(path).second) return;
    log_(LogLevel::Warning, "nie udalo sie wczytac dzwieku \"" + path + "\" - cisza");
}

const AudioMixer::Clip* AudioMixer::LoadClip(const std::string& path) {
    if (auto it = clips_.find(path); it != clips_.end()) return &it->second;
    if (failed_.contains(path)) return nullptr;
    ::Sound sound = ::LoadSound(path.c_str());
    if (!::IsSoundValid(sound)) {
        Failed(path);
        return nullptr;
    }
    return &clips_.emplace(path, Clip{sound}).first->second;
}

std::shared_ptr<const std::vector<unsigned char>> AudioMixer::LoadBytes(const std::string& path) {
    if (auto it = bytes_.find(path); it != bytes_.end()) return it->second;
    if (failed_.contains(path)) return nullptr;
    int size = 0;
    unsigned char* data = ::LoadFileData(path.c_str(), &size);
    if (!data) {
        Failed(path);
        return nullptr;
    }
    auto bytes = std::make_shared<const std::vector<unsigned char>>(data, data + size);
    ::UnloadFileData(data);
    return bytes_.emplace(path, std::move(bytes)).first->second;
}

bool AudioMixer::Start(Slot& slot) {
    const ::Audio& audio = *slot.audio;
    const std::string path = path_(slot.path);
    if (failed_.contains(path)) return false;
    if (audio.IsStreamed() || audio.IsLooped()) {
        if (audio.IsStreamed()) {
            slot.music = ::LoadMusicStream(path.c_str());
        } else {
            slot.bytes = LoadBytes(path);
            if (!slot.bytes) return false;
            // strumień czyta bajty z pamięci przez cały czas gry - slot.bytes je trzyma
            const std::string type = std::filesystem::path(path).extension().string();
            slot.music = ::LoadMusicStreamFromMemory(
                type.c_str(), slot.bytes->data(), static_cast<int>(slot.bytes->size())
            );
        }
        if (!::IsMusicValid(slot.music)) {
            slot.bytes.reset();
            Failed(path);
            return false;
        }
        slot.music.looping = audio.IsLooped();
        slot.stream = true;
    } else {
        const Clip* clip = LoadClip(path);
        if (!clip) return false;
        slot.sound = ::LoadSoundAlias(clip->sound);
    }
    slot.started = true;
    if (audio.FadeInTime() > 0.0f) {
        slot.fade = 0.0f;
        slot.fadeRate = 1.0f / audio.FadeInTime();
    }
    return true;
}

void AudioMixer::Replace(const Slot& slot) {
    if (!Bus(slot.bus).IsExclusive()) return;
    for (Slot& other : slots_)
        if (&other != &slot && other.used && other.started && other.bus == slot.bus) Stop(other);
}

void AudioMixer::Release(Slot& slot, bool notify) {
    if (notify && slot.onFinish) {
        // uchwyt przed zmianą generacji - porównuje się z zapamiętanym w grze
        const auto index = static_cast<std::size_t>(&slot - slots_.data());
        finished_.emplace_back(std::move(slot.onFinish), Handle(index));
    }
    slot.onFinish = nullptr;
    if (slot.started && slot.stream) {
        ::StopMusicStream(slot.music);
        ::UnloadMusicStream(slot.music);
    } else if (slot.started) {
        ::StopSound(slot.sound);
        ::UnloadSoundAlias(slot.sound);
    }
    slot.bytes.reset();
    slot.used = false;
    slot.started = false;
    slot.audio = nullptr;
    // 0 oznacza martwy uchwyt, więc generacja go pomija
    if (++slot.generation == 0) slot.generation = 1;
}

void AudioMixer::Apply(Slot& slot, const BusState& bus) {
    const ::Audio& audio = *slot.audio;
    const float volume = std::max(
        0.0f, audio.Volume() * slot.volume * slot.fade * bus.gain * slot.spatialGain
    );
    const float pitch = std::max(0.01f, audio.Pitch() * slot.variation * slot.pitch);
    const float pan =
        RaylibPan(std::clamp(audio.Pan() + slot.pan + slot.spatialPan, -1.0f, 1.0f));
    if (slot.stream) {
        ::SetMusicVolume(slot.music, volume);
        ::SetMusicPitch(slot.music, pitch);
        ::SetMusicPan(slot.music, pan);
    } else {
        ::SetSoundVolume(slot.sound, volume);
        ::SetSoundPitch(slot.sound, pitch);
        ::SetSoundPan(slot.sound, pan);
    }
}

void AudioMixer::Update(float dt) {
    if (!ready_) return;
    for (Slot& slot : slots_) {
        if (!slot.used) continue;
        const BusState bus = StateOf(slot.bus);
        const bool paused = bus.paused || slot.userPaused;
        if (!slot.started) {
            if (paused) continue; // ruszy po wznowieniu
            if (!Start(slot)) {
                Release(slot);
                continue;
            }
            Replace(slot);
            Apply(slot, bus);
            if (slot.stream)
                ::PlayMusicStream(slot.music);
            else
                ::PlaySound(slot.sound);
            // po starcie: raylib zeruje pozycję przy PlayMusicStream()
            if (slot.seek >= 0.0f) Seek(slot, slot.seek);
            continue;
        }
        if (paused != slot.paused) {
            if (slot.stream && paused) ::PauseMusicStream(slot.music);
            if (slot.stream && !paused) ::ResumeMusicStream(slot.music);
            if (!slot.stream && paused) ::PauseSound(slot.sound);
            if (!slot.stream && !paused) ::ResumeSound(slot.sound);
            slot.paused = paused;
        }
        if (slot.paused) {
            Apply(slot, bus); // np. zmiana głośności w czasie pauzy
            continue;
        }
        if (slot.stream) ::UpdateMusicStream(slot.music);
        const bool playing =
            slot.stream ? ::IsMusicStreamPlaying(slot.music) : ::IsSoundPlaying(slot.sound);
        slot.fade = std::clamp(slot.fade + slot.fadeRate * dt, 0.0f, 1.0f);
        slot.time += dt * slot.audio->Pitch() * slot.variation * slot.pitch;
        if (!playing || (slot.stopping && slot.fade <= 0.0f)) {
            Release(slot); // skończył się albo wyciszył po Stop()
            continue;
        }
        Apply(slot, bus);
    }
    Notify();
}

void AudioMixer::Notify() {
    // kopia: zdarzenie może wywołać Play() albo Stop(), które dopisują kolejne zakończenia
    auto finished = std::exchange(finished_, {});
    for (auto& [callback, voice] : finished)
        callback(voice);
}

std::size_t AudioMixer::Purge() {
    std::set<std::string> playing;
    for (const Slot& slot : slots_)
        if (slot.used) playing.insert(path_(slot.path));
    std::size_t released = 0;
    for (auto it = clips_.begin(); it != clips_.end();) {
        if (playing.contains(it->first)) {
            ++it;
            continue;
        }
        ::UnloadSound(it->second.sound);
        it = clips_.erase(it);
        ++released;
    }
    released += std::erase_if(bytes_, [&playing](const auto& entry) {
        return !playing.contains(entry.first);
    });
    failed_.clear(); // następna próba wczyta brakujący plik (np. po skopiowaniu)
    return released;
}
