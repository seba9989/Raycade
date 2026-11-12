/**
 * @file AudioMixer.cpp
 * @brief Mikser silnika: urządzenie, pula głosów, dźwięki i szyny.
 */

#include "AudioMixer.hpp"

#include <algorithm>
#include <utility>

namespace {

/** @brief Rozmiar puli głosów. */
constexpr std::size_t kSlots = 64;

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
    buses_.try_emplace("", *this, "");
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
        if (slot.used) Release(slot);
    for (auto& [path, clip] : clips_)
        ::UnloadSound(clip.sound);
    clips_.clear();
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
    auto free = std::ranges::find_if(slots_, [](const Slot& slot) { return !slot.used; });
    if (free == slots_.end()) {
        log_(LogLevel::Warning, "pula glosow pelna - pominieto \"" + audio.Name() + "\"");
        return {};
    }
    Slot& slot = *free;
    const std::uint32_t generation = slot.generation;
    slot = Slot{};
    slot.generation = generation;
    slot.used = true;
    slot.audio = &audio;
    slot.bus = audio.Bus();
    slot.path = PickFile(audio);
    slot.variation = 1.0f + audio.PitchVariation() * RandomUnit();
    return Handle(static_cast<std::size_t>(free - slots_.begin()));
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

void AudioMixer::Stop(Slot& slot) {
    Release(slot);
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
        return slot.used && slot.audio == &audio;
    }));
}

int AudioMixer::VoiceCount(const ::AudioBus& bus) const {
    return static_cast<int>(std::ranges::count_if(slots_, [this, &bus](const Slot& slot) {
        return slot.used && IsWithin(slot.bus, bus.Name());
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

const AudioMixer::Clip* AudioMixer::LoadClip(const std::string& path) {
    if (auto it = clips_.find(path); it != clips_.end()) return &it->second;
    if (failed_.contains(path)) return nullptr;
    ::Sound sound = ::LoadSound(path.c_str());
    if (!::IsSoundValid(sound)) {
        failed_.insert(path);
        log_(LogLevel::Warning, "nie udalo sie wczytac dzwieku \"" + path + "\" - cisza");
        return nullptr;
    }
    return &clips_.emplace(path, Clip{sound}).first->second;
}

bool AudioMixer::Start(Slot& slot) {
    const Clip* clip = LoadClip(path_(slot.path));
    if (!clip) return false;
    slot.sound = ::LoadSoundAlias(clip->sound);
    slot.started = true;
    return true;
}

void AudioMixer::Release(Slot& slot) {
    if (slot.started) {
        ::StopSound(slot.sound);
        ::UnloadSoundAlias(slot.sound);
    }
    slot.used = false;
    slot.started = false;
    slot.audio = nullptr;
    // 0 oznacza martwy uchwyt, więc generacja go pomija
    if (++slot.generation == 0) slot.generation = 1;
}

void AudioMixer::Apply(Slot& slot, const BusState& bus) {
    const ::Audio& audio = *slot.audio;
    ::SetSoundVolume(slot.sound, std::max(0.0f, audio.Volume() * slot.volume * bus.gain));
    ::SetSoundPitch(slot.sound, std::max(0.01f, audio.Pitch() * slot.variation * slot.pitch));
    ::SetSoundPan(slot.sound, RaylibPan(std::clamp(audio.Pan() + slot.pan, -1.0f, 1.0f)));
}

void AudioMixer::Update([[maybe_unused]] float dt) {
    if (!ready_) return;
    for (Slot& slot : slots_) {
        if (!slot.used) continue;
        const BusState bus = StateOf(slot.bus);
        if (!slot.started) {
            if (bus.paused) continue; // ruszy po wznowieniu szyny
            if (!Start(slot)) {
                Release(slot);
                continue;
            }
            Apply(slot, bus);
            ::PlaySound(slot.sound);
            continue;
        }
        if (bus.paused != slot.paused) {
            if (bus.paused)
                ::PauseSound(slot.sound);
            else
                ::ResumeSound(slot.sound);
            slot.paused = bus.paused;
        }
        if (!slot.paused && !::IsSoundPlaying(slot.sound)) {
            Release(slot); // skończył się
            continue;
        }
        Apply(slot, bus);
    }
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
    failed_.clear(); // następna próba wczyta brakujący plik (np. po skopiowaniu)
    return released;
}
