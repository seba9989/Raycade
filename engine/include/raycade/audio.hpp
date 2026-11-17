/**
 * @file audio.hpp
 * @brief Zbiorczy nagłówek dźwięku: mikser (Audio, audio::Sound, Music, Bus, Voice).
 * @details Dołączany przez `<raycade>`. Obiekty sceny (Emitter, Listener) dołącza `node/sound.hpp`.
 */
#pragma once

/**
 * @namespace audio
 * @brief Mikser dźwięku: dźwięki, muzyka, szyny i odtworzenia.
 * @details Dźwięki, muzykę i szyny miksera pobiera się po nazwie z fasady silnika
 * (Engine::Audio()):
 * - @ref audio::Sound - efekt grany wiele razy naraz (`Play()` zwraca @ref audio::Voice),
 * - @ref audio::Music - ścieżka muzyczna, jedna naraz, z pauzą i przewijaniem na sobie,
 * - @ref audio::Source - ustawienia wspólne dla obu,
 * - @ref audio::Bus - szyna miksera (grupa dźwięków ze wspólną głośnością).
 *
 * Obiekty sceny, które odtwarzają dźwięki w świecie gry (Emitter, Listener), są w przestrzeni
 * nazw @ref node::sound.
 */

// IWYU pragma: begin_exports
#include <raycade/assets/sound/AudioClip.hpp>
#include <raycade/audio/Audio.hpp>
#include <raycade/audio/mixing/Bus.hpp>
#include <raycade/audio/playback/Music.hpp>
#include <raycade/audio/playback/Sound.hpp>
#include <raycade/audio/playback/Source.hpp>
#include <raycade/audio/playback/Voice.hpp>
// IWYU pragma: end_exports
