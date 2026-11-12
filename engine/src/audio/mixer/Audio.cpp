/**
 * @file Audio.cpp
 * @brief Ustawienia dźwięku i przekazanie poleceń do miksera silnika.
 */

#include <raycade/audio/mixer/Audio.hpp>

#include <algorithm>
#include <utility>

#include "AudioMixer.hpp"

Audio& Audio::Files(std::vector<std::string> files) {
    files_ = std::move(files);
    lastFile_ = -1;
    return *this;
}

Audio& Audio::Volume(float volume) {
    volume_ = std::max(0.0f, volume);
    return *this;
}

Audio& Audio::Pitch(float pitch) {
    pitch_ = std::max(0.01f, pitch);
    return *this;
}

Audio& Audio::PitchVariation(float variation) {
    pitchVariation_ = std::clamp(variation, 0.0f, 0.99f);
    return *this;
}

Audio& Audio::Pan(float pan) {
    pan_ = std::clamp(pan, -1.0f, 1.0f);
    return *this;
}

Audio& Audio::FadeInTime(float seconds) {
    fadeInTime_ = std::max(0.0f, seconds);
    return *this;
}

Audio& Audio::FadeOutTime(float seconds) {
    fadeOutTime_ = std::max(0.0f, seconds);
    return *this;
}

Audio& Audio::Bus(std::string bus) {
    mixer_->Bus(bus); // szyna powstaje od razu - widać ją w logu przy konfiguracji
    bus_ = std::move(bus);
    return *this;
}

Voice Audio::Play() {
    return mixer_->Play(*this);
}

Audio& Audio::Stop() {
    mixer_->Stop(*this);
    return *this;
}

int Audio::VoiceCount() const {
    return mixer_->VoiceCount(*this);
}
