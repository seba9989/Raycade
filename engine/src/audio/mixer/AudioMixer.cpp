/**
 * @file AudioMixer.cpp
 * @brief Mikser silnika: urządzenie, pula głosów, dźwięki, muzyka i szyny.
 */

#include "AudioMixer.hpp"

#include <raycade/render/Assets.hpp>

#include <algorithm>
#include <utility>

namespace {

/** @brief Rozmiar puli głosów (największy limit szyny głównej). */
constexpr std::size_t kSlots = 128;

/** @brief Domyślny limit głosów szyny głównej. */
constexpr int kDefaultVoices = 32;

/** @brief Najgłębsze drzewo szyn (ochrona przed cyklem przy liczeniu stanu). */
constexpr int kMaxBusDepth = 64;

/** @brief Klucz błędu strumienia w Assets (inny niż klucz klipu tego samego pliku). */
std::string StreamKey(const std::string& path) {
    return path + "\x1fstream";
}

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

using audio::Voice;

AudioMixer::AudioMixer(LogFunction log, ::Assets& assets)
    : log_(std::move(log)), assets_(assets), slots_(kSlots) {
    buses_.try_emplace("", *this, "").first->second.maxVoices_ = kDefaultVoices;
    buses_.try_emplace(::Audio::kMusicBus, *this, ::Audio::kMusicBus).first->second.Exclusive();
    current_ = this;
}

AudioMixer::~AudioMixer() {
    StopAll();
    CloseDevice();
    if (current_ == this) current_ = nullptr;
}

void AudioMixer::Open() {
    if (ready_) return;
    ::InitAudioDevice();
    ready_ = ::IsAudioDeviceReady();
    if (!ready_) log_(LogLevel::Warning, "nie udalo sie otworzyc urzadzenia audio - bez dzwieku");
}

void AudioMixer::StopAll() {
    for (Slot& slot : slots_)
        if (slot.used) Release(slot, false);
    finished_.clear();
    finishedMusic_.clear();
}

void AudioMixer::CloseDevice() {
    if (ready_) ::CloseAudioDevice();
    ready_ = false;
}

bool AudioMixer::IsLoaded(const audio::Source& source) const {
    const std::vector<std::string> single{source.Name()};
    for (const std::string& file : source.files_.empty() ? single : source.files_) {
        if (source.streamed_) {
            // strumień czyta plik dopiero przy graniu - tu tylko, czy plik istnieje
            if (!::FileExists(assets_.Path(file).c_str())) return false;
        } else if (!assets_.Clip(file)->IsLoaded()) {
            return false;
        }
    }
    return true;
}

audio::Sound& AudioMixer::Sound(const std::string& name) {
    return sounds_.try_emplace(name, *this, name).first->second;
}

audio::Music& AudioMixer::Music(const std::string& name) {
    return music_.try_emplace(name, *this, name, ::Audio::kMusicBus).first->second;
}

audio::Bus& AudioMixer::Bus(const std::string& name) {
    auto [it, created] = buses_.try_emplace(name, *this, name);
    if (created) log_(LogLevel::Debug, "nowa szyna audio \"" + name + "\"");
    return it->second;
}

bool AudioMixer::WarnNoDevice() {
    if (ready_) return false;
    if (!warnedNoDevice_)
        log_(LogLevel::Warning, "Play() bez urzadzenia audio - dzwieki sa wylaczone");
    warnedNoDevice_ = true;
    return true;
}

Voice AudioMixer::Play(audio::Sound& sound) {
    if (WarnNoDevice()) return {};
    const double now = ::GetTime();
    if (sound.MinInterval() > 0.0f && now - sound.lastPlay_ < sound.MinInterval()) return {};
    sound.lastPlay_ = now;
    const Voice voice = Request(sound, sound.MaxVoices());
    if (Slot* slot = Find(voice); slot && !slot->sound) {
        slot->sound = &sound;
        slot->variation = 1.0f + sound.PitchVariation() * RandomUnit();
    }
    return voice;
}

void AudioMixer::Play(audio::Music& music) {
    if (WarnNoDevice()) return;
    // ścieżka już gra (także wstrzymana) - nie zaczyna się od nowa
    if (const Slot* slot = Find(music.voice_); slot && !slot->stopping) return;
    music.voice_ = Request(music, 0);
    if (Slot* slot = Find(music.voice_)) slot->track = &music;
}

/**
 * @details Na szynie Exclusive() trwające odtworzenie tego samego dźwięku jest zwracane zamiast
 * nowego, a zgłoszone w tej klatce inne dźwięki ustępują ostatniemu Play().
 */
Voice AudioMixer::Request(audio::Source& source, int maxVoices) {
    const std::string& bus = source.Bus();
    if (Bus(bus).IsExclusive()) {
        for (std::size_t i = 0; i < slots_.size(); ++i) {
            Slot& other = slots_[i];
            if (!other.used || other.bus != bus || other.stopping) continue;
            if (other.source == &source) return Handle(i);
            if (!other.started) Release(other, false);
        }
    }
    MakeRoom(source, maxVoices);
    auto free = std::ranges::find_if(slots_, [](const Slot& slot) { return !slot.used; });
    if (free == slots_.end()) return {}; // nie zdarza się: MakeRoom() zwalnia miejsce
    Slot& slot = *free;
    const std::uint32_t generation = slot.generation;
    slot = Slot{};
    slot.generation = generation;
    slot.used = true;
    slot.source = &source;
    slot.bus = bus;
    slot.path = PickFile(source);
    slot.order = ++order_;
    // głos należy do sceny, z której go zagrano - chyba że dźwięk jest globalny
    slot.owner = source.IsGlobal() ? nullptr : owner_;
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
 * @details Kolejno: limit dźwięku (maxVoices), limity szyny dźwięku i jej rodziców (aż do szyny głównej,
 * której limit to rozmiar puli) i na końcu sama pula. W każdym przypadku ustępuje najstarszy
 * głos z danej grupy - nowy dźwięk jest zwykle ważniejszy niż ten, który gra od dawna.
 */
void AudioMixer::MakeRoom(const audio::Source& source, int maxVoices) {
    if (maxVoices > 0)
        while (VoiceCount(source) >= maxVoices &&
               StealOldest([&source](const Slot& s) { return s.source == &source; })) {}
    std::string bus = source.Bus();
    for (int depth = 0; depth < kMaxBusDepth; ++depth) {
        const audio::Bus& node = Bus(bus);
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

int AudioMixer::ClampVoices(const audio::Bus& bus, int count) const {
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
             "PlaybackTime(s) dziala tylko dla muzyki i dzwiekow Streamed() albo Looped() - "
             "pominieto");
    warnedSeek_ = true;
}

float AudioMixer::PlaybackTime(const Slot& slot) const {
    if (!slot.started) return 0.0f;
    return slot.stream ? ::GetMusicTimePlayed(slot.music) : slot.time;
}

float AudioMixer::Duration(const Slot& slot) const {
    if (!slot.started) return 0.0f;
    if (slot.stream) return ::GetMusicTimeLength(slot.music);
    const unsigned int rate = slot.alias.stream.sampleRate;
    return rate > 0 ? static_cast<float>(slot.alias.frameCount) / static_cast<float>(rate) : 0.0f;
}

std::string AudioMixer::PickFile(audio::Source& source) {
    const auto& files = source.files_;
    if (files.empty()) return source.Name();
    if (files.size() == 1) return files.front();
    // bez powtórzeń: losujemy spośród pozostałych plików i przeskakujemy poprzedni
    const int count = static_cast<int>(files.size());
    int index = ::GetRandomValue(0, source.lastFile_ >= 0 ? count - 2 : count - 1);
    if (source.lastFile_ >= 0 && index >= source.lastFile_) ++index;
    source.lastFile_ = index;
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
    const float fadeOut = slot.source->FadeOutTime();
    if (!slot.started || slot.paused || fadeOut <= 0.0f || slot.fade <= 0.0f) {
        Release(slot);
        return;
    }
    slot.stopping = true;
    slot.fadeRate = -1.0f / fadeOut;
}

void AudioMixer::Stop(const audio::Source& source) {
    for (Slot& slot : slots_)
        if (slot.used && slot.source == &source) Stop(slot);
}

void AudioMixer::Stop(const audio::Bus& bus) {
    for (Slot& slot : slots_)
        if (slot.used && IsWithin(slot.bus, bus.Name())) Stop(slot);
}

int AudioMixer::VoiceCount(const audio::Source& source) const {
    return static_cast<int>(std::ranges::count_if(slots_, [&source](const Slot& slot) {
        return slot.used && !slot.stopping && slot.source == &source;
    }));
}

int AudioMixer::VoiceCount(const audio::Bus& bus) const {
    return static_cast<int>(std::ranges::count_if(slots_, [this, &bus](const Slot& slot) {
        return slot.used && !slot.stopping && IsWithin(slot.bus, bus.Name());
    }));
}

void AudioMixer::Reparent(audio::Bus& bus, const std::string& parent) {
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
        const audio::Bus& node = it->second;
        state.gain *= node.IsMuted() ? 0.0f : node.Volume();
        state.paused = state.paused || node.IsPaused();
        if (current.empty()) break; // szyna główna - koniec drzewa
        current = node.Parent();
    }
    return state;
}

bool AudioMixer::Start(Slot& slot) {
    const audio::Source& source = *slot.source;
    if (source.streamed_) {
        // każdy głos czyta plik osobno - nie ma czego współdzielić przez Assets
        const std::string path = assets_.Path(slot.path);
        const std::string key = StreamKey(path);
        if (assets_.HasFailed(key)) return false;
        slot.music = ::LoadMusicStream(path.c_str());
        if (!::IsMusicValid(slot.music)) {
            assets_.Report(key, "dzwieku", path, "cisza");
            return false;
        }
        slot.music.looping = source.IsLooped();
        slot.stream = true;
    } else {
        slot.clip = assets_.Clip(slot.path); // błąd zapisuje Assets (raz na plik)
        if (!slot.clip->IsLoaded()) {
            slot.clip.reset();
            return false;
        }
        if (source.IsLooped()) {
            // strumień czyta bajty klipu przez cały czas gry - slot.clip je trzyma
            const auto bytes = slot.clip->Bytes();
            slot.music = ::LoadMusicStreamFromMemory(
                slot.clip->Type().c_str(), bytes.data(), static_cast<int>(bytes.size())
            );
            if (!::IsMusicValid(slot.music)) {
                const std::string path = assets_.Path(slot.path);
                assets_.Report(StreamKey(path), "dzwieku", path, "cisza");
                slot.clip.reset();
                return false;
            }
            slot.music.looping = true;
            slot.stream = true;
        } else {
            slot.alias = ::LoadSoundAlias(slot.clip->Sound());
        }
    }
    slot.started = true;
    if (source.FadeInTime() > 0.0f) {
        slot.fade = 0.0f;
        slot.fadeRate = 1.0f / source.FadeInTime();
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
        ::StopSound(slot.alias);
        ::UnloadSoundAlias(slot.alias);
    }
    slot.clip.reset(); // po aliasie i strumieniu - oba korzystają z danych klipu
    slot.used = false;
    slot.started = false;
    slot.source = nullptr;
    slot.sound = nullptr;
    slot.track = nullptr;
    // 0 oznacza martwy uchwyt, więc generacja go pomija
    if (++slot.generation == 0) slot.generation = 1;
}

void AudioMixer::Apply(Slot& slot, const BusState& bus) {
    const audio::Source& source = *slot.source;
    const float volume = std::max(
        0.0f, source.Volume() * slot.volume * slot.fade * bus.gain * slot.spatialGain
    );
    const float pitch = std::max(0.01f, source.Pitch() * slot.variation * slot.pitch);
    const float pan =
        RaylibPan(std::clamp(source.Pan() + slot.pan + slot.spatialPan, -1.0f, 1.0f));
    if (slot.stream) {
        ::SetMusicVolume(slot.music, volume);
        ::SetMusicPitch(slot.music, pitch);
        ::SetMusicPan(slot.music, pan);
    } else {
        ::SetSoundVolume(slot.alias, volume);
        ::SetSoundPitch(slot.alias, pitch);
        ::SetSoundPan(slot.alias, pan);
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
                ::PlaySound(slot.alias);
            // po starcie: raylib zeruje pozycję przy PlayMusicStream()
            if (slot.seek >= 0.0f) Seek(slot, slot.seek);
            continue;
        }
        if (paused != slot.paused) {
            if (slot.stream && paused) ::PauseMusicStream(slot.music);
            if (slot.stream && !paused) ::ResumeMusicStream(slot.music);
            if (!slot.stream && paused) ::PauseSound(slot.alias);
            if (!slot.stream && !paused) ::ResumeSound(slot.alias);
            slot.paused = paused;
        }
        if (slot.paused) {
            Apply(slot, bus); // np. zmiana głośności w czasie pauzy
            continue;
        }
        if (slot.stream) ::UpdateMusicStream(slot.music);
        const bool playing =
            slot.stream ? ::IsMusicStreamPlaying(slot.music) : ::IsSoundPlaying(slot.alias);
        slot.fade = std::clamp(slot.fade + slot.fadeRate * dt, 0.0f, 1.0f);
        slot.time += dt * slot.source->Pitch() * slot.variation * slot.pitch;
        // koniec pliku muzyki bez pętli (nie Stop() ani zastąpienie) - Music::OnFinish()
        if (!playing && !slot.stopping && slot.track && slot.track->onFinish_)
            finishedMusic_.push_back(slot.track);
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
    for (audio::Music* music : std::exchange(finishedMusic_, {}))
        if (music->onFinish_) music->onFinish_(*music);
}

