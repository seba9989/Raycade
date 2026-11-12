/**
 * @file EngineAudio.cpp
 * @brief Dźwięk: dostęp do dźwięków i szyn miksera silnika.
 */

#include <raycade/Engine.hpp>

#include "../audio/mixer/AudioMixer.hpp"

Audio& Engine::Audio(const std::string& name) {
    return mixer_->Sound(name);
}

AudioBus& Engine::AudioBus(const std::string& name) {
    return mixer_->Bus(name);
}
