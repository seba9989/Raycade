/**
 * @file EngineAudio.cpp
 * @brief Dźwięk: fasada miksera silnika (Audio).
 */

#include <raycade/Engine.hpp>

#include "../audio/mixer/AudioMixer.hpp"

Audio& Engine::Audio() {
    return mixer_->Facade();
}

const Audio& Engine::Audio() const {
    return mixer_->Facade();
}
