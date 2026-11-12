/**
 * @file AudioBus.cpp
 * @brief Ustawienia szyny i przekazanie poleceń do miksera silnika.
 */

#include <raycade/audio/mixer/AudioBus.hpp>

#include <algorithm>
#include <utility>

#include "AudioMixer.hpp"

AudioBus& AudioBus::Parent(std::string parent) {
    mixer_->Reparent(*this, parent);
    return *this;
}

AudioBus& AudioBus::Volume(float volume) {
    volume_ = std::max(0.0f, volume);
    return *this;
}

AudioBus& AudioBus::Stop() {
    mixer_->Stop(*this);
    return *this;
}

int AudioBus::VoiceCount() const {
    return mixer_->VoiceCount(*this);
}
