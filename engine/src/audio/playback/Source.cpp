/**
 * @file Source.cpp
 * @brief audio::Source, audio::Sound i audio::Music - przekazanie poleceń do miksera silnika.
 */

#include <raycade/audio/playback/Music.hpp>
#include <raycade/audio/playback/Sound.hpp>
#include <raycade/audio/playback/Source.hpp>

#include <utility>

#include "../AudioMixer.hpp"

namespace audio {

void Source::UseBus(std::string bus) {
    mixer_->Bus(bus); // szyna powstaje od razu - widać ją w logu przy konfiguracji
    bus_ = std::move(bus);
}

void Source::StopVoices() {
    mixer_->Stop(*this);
}

bool Source::IsLoaded() const {
    return mixer_->IsLoaded(*this);
}

int Source::VoiceCount() const {
    return mixer_->VoiceCount(*this);
}

Voice Sound::Play() {
    return mixer_->Play(*this);
}

Music& Music::Play() {
    mixer_->Play(*this);
    return *this;
}

Music& Music::Pause() {
    voice_.Pause();
    return *this;
}

Music& Music::Resume() {
    voice_.Play();
    return *this;
}

bool Music::IsPaused() const {
    return voice_.IsPaused();
}

Music& Music::PlaybackTime(float seconds) {
    voice_.PlaybackTime(seconds);
    return *this;
}

float Music::PlaybackTime() const {
    return voice_.PlaybackTime();
}

float Music::Duration() const {
    return voice_.Duration();
}

} // namespace audio
