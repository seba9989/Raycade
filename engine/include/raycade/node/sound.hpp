/**
 * @file sound.hpp
 * @brief Zbiorczy nagłówek obiektów sceny odtwarzających dźwięk: Emitter i Listener.
 * @details Dołączany przez `<raycade>`. Dźwięki, muzykę i szyny miksera dołącza `audio.hpp`.
 */
#pragma once

/**
 * @namespace node::sound
 * @brief Obiekty sceny, które odtwarzają dźwięki w świecie gry.
 * @details
 * - @ref node::sound::Emitter - źródło dźwięku: tłumienie z odległością i panorama (`Sound()`,
 *   `MaxDistance()`, `Attenuation()`, `Play()`, `Pause()`, `Stop()`, `OnFinish()`),
 * - @ref node::sound::Listener - punkt, z którego słychać emitery (zwykle dziecko postaci gracza).
 *
 * Same dźwięki (audio::Sound, audio::Music, audio::Bus) są w przestrzeni nazw @ref audio.
 * Przestrzeń nazywa się `sound`, a nie `audio`, żeby `using namespace node;` nie robiło
 * z `audio::` nazwy niejednoznacznej.
 */

// IWYU pragma: begin_exports
#include <raycade/node/sound/Emitter.hpp>
#include <raycade/node/sound/Listener.hpp>
// IWYU pragma: end_exports
