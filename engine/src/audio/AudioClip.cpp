/**
 * @file AudioClip.cpp
 * @brief Wczytywanie pliku dźwiękowego, wysyłanie próbek do urządzenia i zwalnianie.
 */

#include <raycade/audio/AudioClip.hpp>

#include <filesystem>
#include <utility>

std::optional<AudioClip> AudioClip::Load(const std::string& path) {
    int size = 0;
    unsigned char* data = ::LoadFileData(path.c_str(), &size);
    if (data == nullptr || size <= 0) {
        if (data != nullptr) ::UnloadFileData(data);
        return std::nullopt;
    }
    AudioClip clip;
    clip.bytes_.assign(data, data + size);
    ::UnloadFileData(data);
    clip.type_ = std::filesystem::path(path).extension().string();
    // dekodowanie od razu: błąd formatu wychodzi przy wczytaniu, a nie przy pierwszym Play()
    clip.pending_ = ::LoadWaveFromMemory(
        clip.type_.c_str(), clip.bytes_.data(), static_cast<int>(clip.bytes_.size())
    );
    if (!::IsWaveValid(clip.pending_)) return std::nullopt; // destruktor zwolni resztę
    clip.duration_ =
        static_cast<float>(clip.pending_.frameCount) / static_cast<float>(clip.pending_.sampleRate);
    return clip;
}

AudioClip::~AudioClip() {
    Release();
}

AudioClip::AudioClip(AudioClip&& other) noexcept
    : bytes_(std::move(other.bytes_)), type_(std::move(other.type_)),
      duration_(std::exchange(other.duration_, 0.0f)), pending_(std::exchange(other.pending_, {})),
      sound_(std::exchange(other.sound_, {})) {
    other.bytes_.clear();
}

AudioClip& AudioClip::operator=(AudioClip&& other) noexcept {
    if (this != &other) {
        Release();
        bytes_ = std::move(other.bytes_);
        other.bytes_.clear();
        type_ = std::move(other.type_);
        duration_ = std::exchange(other.duration_, 0.0f);
        pending_ = std::exchange(other.pending_, {});
        sound_ = std::exchange(other.sound_, {});
    }
    return *this;
}

const ::Sound& AudioClip::Sound() const {
    if (sound_.stream.buffer == nullptr && pending_.data != nullptr && ::IsAudioDeviceReady()) {
        sound_ = ::LoadSoundFromWave(pending_);
        ::UnloadWave(pending_); // próbki są już w urządzeniu
        pending_ = {};
    }
    return sound_;
}

void AudioClip::Release() {
    // próbki w urządzeniu można zwolnić tylko przed CloseAudioDevice() - po nim raylib nie ma
    // już blokady listy buforów (tak jak Bitmap nie zwalnia tekstury po zamknięciu okna)
    if (sound_.stream.buffer != nullptr && ::IsAudioDeviceReady()) ::UnloadSound(sound_);
    sound_ = {};
    if (pending_.data != nullptr) ::UnloadWave(pending_);
    pending_ = {};
    bytes_.clear();
}
