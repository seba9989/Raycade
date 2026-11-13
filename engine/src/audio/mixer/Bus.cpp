/**
 * @file Bus.cpp
 * @brief Ustawienia szyny i przekazanie poleceń do miksera silnika.
 */

#include <raycade/audio/mixer/Bus.hpp>

#include <algorithm>
#include <utility>

#include "AudioMixer.hpp"

namespace audio {

Bus& Bus::Parent(std::string parent) {
    mixer_->Reparent(*this, parent);
    return *this;
}

Bus& Bus::Volume(float volume) {
    volume_ = std::max(0.0f, volume);
    return *this;
}

Bus& Bus::MaxVoices(int count) {
    maxVoices_ = mixer_->ClampVoices(*this, count);
    return *this;
}

Bus& Bus::Stop() {
    mixer_->Stop(*this);
    return *this;
}

int Bus::VoiceCount() const {
    return mixer_->VoiceCount(*this);
}

} // namespace audio
