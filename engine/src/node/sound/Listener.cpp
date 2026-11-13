/**
 * @file Listener.cpp
 * @brief audio::Listener - zapis położenia słuchacza w mikserze.
 */

#include <raycade/node/sound/Listener.hpp>

#include "../../audio/AudioMixer.hpp"

namespace audio {

Listener::~Listener() {
    if (AudioMixer* mixer = AudioMixer::Current()) mixer->Unlisten(this);
}

void Listener::OnMount() {
    if (AudioMixer* mixer = AudioMixer::Current())
        mixer->Listen(this, OwnerScene(), WorldPosition());
}

void Listener::Tick(float, const Input&) {
    if (AudioMixer* mixer = AudioMixer::Current())
        mixer->Listen(this, OwnerScene(), WorldPosition());
}

} // namespace audio
