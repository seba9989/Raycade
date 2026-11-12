/**
 * @file audio.hpp
 * @brief Zbiorczy nagłówek dźwięku: mikser (Audio, AudioBus, Voice) i obiekty sceny.
 * @details Dołączany przez `<raycade>`.
 */
#pragma once

/**
 * @namespace audio
 * @brief Obiekty sceny związane z dźwiękiem: źródła w przestrzeni 2D i słuchacz.
 * @details Dźwięki i szyny miksera pobiera się z silnika (Engine::Audio(), Engine::AudioBus());
 * obiekty z tej przestrzeni nazw odtwarzają je w świecie gry:
 * - @ref audio::Emitter - źródło dźwięku: tłumienie z odległością i panorama (`Audio()`,
 *   `MaxDistance()`, `Attenuation()`, `Play()`, `Pause()`, `Stop()`, `OnFinish()`),
 * - @ref audio::Listener - punkt, z którego słychać emitery (zwykle dziecko postaci gracza).
 */

// IWYU pragma: begin_exports
#include <raycade/audio/Emitter.hpp>
#include <raycade/audio/Listener.hpp>
#include <raycade/audio/mixer/Audio.hpp>
#include <raycade/audio/mixer/AudioBus.hpp>
#include <raycade/audio/mixer/Voice.hpp>
// IWYU pragma: end_exports
