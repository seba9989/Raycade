/**
 * @file audio.hpp
 * @brief Zbiorczy nagłówek dźwięku: mikser (Audio, audio::Sound, Music, Bus, Voice) i obiekty sceny.
 * @details Dołączany przez `<raycade>`.
 */
#pragma once

/**
 * @namespace audio
 * @brief Dźwięk: dźwięki, muzyka, szyny, odtworzenia i obiekty sceny (źródła i słuchacz).
 * @details Dźwięki, muzykę i szyny miksera pobiera się po nazwie z fasady silnika
 * (Engine::Audio()):
 * - @ref audio::Sound - efekt grany wiele razy naraz (`Play()` zwraca @ref audio::Voice),
 * - @ref audio::Music - ścieżka muzyczna, jedna naraz, z pauzą i przewijaniem na sobie,
 * - @ref audio::Source - ustawienia wspólne dla obu,
 * - @ref audio::Bus - szyna miksera (grupa dźwięków ze wspólną głośnością).
 *
 * Obiekty sceny z tej przestrzeni nazw odtwarzają dźwięki w świecie gry:
 * - @ref audio::Emitter - źródło dźwięku: tłumienie z odległością i panorama (`Sound()`,
 *   `MaxDistance()`, `Attenuation()`, `Play()`, `Pause()`, `Stop()`, `OnFinish()`),
 * - @ref audio::Listener - punkt, z którego słychać emitery (zwykle dziecko postaci gracza).
 */

// IWYU pragma: begin_exports
#include <raycade/audio/AudioClip.hpp>
#include <raycade/audio/Emitter.hpp>
#include <raycade/audio/Listener.hpp>
#include <raycade/audio/mixer/Audio.hpp>
#include <raycade/audio/mixer/Bus.hpp>
#include <raycade/audio/mixer/Music.hpp>
#include <raycade/audio/mixer/Sound.hpp>
#include <raycade/audio/mixer/Source.hpp>
#include <raycade/audio/mixer/Voice.hpp>
// IWYU pragma: end_exports
