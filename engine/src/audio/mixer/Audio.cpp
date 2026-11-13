/**
 * @file Audio.cpp
 * @brief Fasada dźwięku silnika - przekazuje wywołania do miksera.
 */

#include <raycade/audio/mixer/Audio.hpp>

#include "AudioMixer.hpp"

audio::Sound& Audio::Sound(const std::string& name) {
    return mixer_->Sound(name);
}

audio::Music& Audio::Music(const std::string& name) {
    return mixer_->Music(name);
}

audio::Bus& Audio::Bus(const std::string& name) {
    return mixer_->Bus(name);
}

const audio::Bus& Audio::Master() const {
    return mixer_->Bus("");
}

bool Audio::IsReady() const {
    return mixer_->IsReady();
}

Audio& Audio::Volume(float volume) {
    Bus().Volume(volume);
    return *this;
}

float Audio::Volume() const {
    return Master().Volume();
}

Audio& Audio::Muted(bool value) {
    Bus().Muted(value);
    return *this;
}

bool Audio::IsMuted() const {
    return Master().IsMuted();
}

Audio& Audio::Paused(bool value) {
    Bus().Paused(value);
    return *this;
}

bool Audio::IsPaused() const {
    return Master().IsPaused();
}

Audio& Audio::MaxVoices(int count) {
    Bus().MaxVoices(count);
    return *this;
}

int Audio::MaxVoices() const {
    return Master().MaxVoices();
}

Audio& Audio::Stop() {
    Bus().Stop();
    return *this;
}

int Audio::VoiceCount() const {
    return Master().VoiceCount();
}
