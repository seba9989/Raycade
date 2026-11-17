/**
 * @file EngineAudio.cpp
 * @brief Dźwięk: fasada miksera silnika (Audio).
 */

#include <raycade/engine/Engine.hpp>

#include "../audio/AudioMixer.hpp"

Audio& Engine::Audio() {
    return mixer_->Facade();
}

const Audio& Engine::Audio() const {
    return mixer_->Facade();
}
