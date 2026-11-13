/**
 * @file Music.hpp
 * @brief Klasa audio::Music - ścieżka muzyczna (jedno odtworzenie naraz, strumień).
 */
#pragma once

#include <functional>
#include <string>
#include <utility>

#include <raycade/audio/mixer/Source.hpp>
#include <raycade/audio/mixer/Voice.hpp>

namespace audio {

/**
 * @brief Muzyka: ścieżka czytana z pliku na bieżąco, grana jednym odtworzeniem naraz.
 * @details Muzykę pobiera się po nazwie: `engine.Audio().Music("theme.ogg")`. Domyślnie gra
 * w pętli, na szynie "music" (Audio::kMusicBus, z Bus::Exclusive()) i niezależnie od scen
 * (Global()) - więc nowa ścieżka płynnie zastępuje poprzednią, a zmiana sceny jej nie urywa:
 * @code{.cpp}
 * engine.Audio().Music("theme.ogg").FadeInTime(1.5f).FadeOutTime(1.0f);
 * engine.Audio().Music("boss.ogg").FadeInTime(0.5f);
 * // ...
 * engine.Audio().Music("boss.ogg").Play();  // theme.ogg się wycisza, boss.ogg narasta
 * engine.Audio().Music("boss.ogg").Pause(); // menu pauzy
 * @endcode
 *
 * Muzyka ma jedno odtworzenie, więc pauza, przewijanie i koniec są na niej samej, bez uchwytu
 * audio::Voice. Krótka ścieżka bez pętli (dżingiel) to `Looped(false)` z OnFinish().
 *
 * Nazwy muzyki są osobne od nazw dźwięków (Audio::Sound()) - ten sam plik może być i tym,
 * i tym. W `using namespace audio;` nazwa Music jest niejednoznaczna z typem raylib `::Music`.
 */
class Music : public Source {
  public:
    /**
     * @brief Tworzy muzykę (robi to Audio::Music(), gra nie tworzy muzyki sama).
     * @param mixer Mikser silnika.
     * @param name Nazwa ścieżki.
     * @param bus Szyna domyślna (Audio::kMusicBus).
     */
    Music(AudioMixer& mixer, std::string name, std::string bus)
        : Source(mixer, std::move(name), std::move(bus)) {
        looped_ = true;
        streamed_ = true;
        global_ = true;
    }

    /**
     * @brief Odtwarza ścieżkę od początku.
     * @details Gdy ścieżka już gra (także wstrzymana), nic nie robi - nie zaczyna od nowa.
     * Na szynie Exclusive() wycisza inne ścieżki tej szyny (ich FadeOutTime()). Odtworzenie
     * startuje na końcu klatki; bez urządzenia audio nic nie robi.
     * @return Ta sama muzyka.
     */
    Music& Play();

    /**
     * @brief Wstrzymuje ścieżkę w bieżącym miejscu (wznawia ją Resume()).
     * @return Ta sama muzyka.
     */
    Music& Pause();

    /**
     * @brief Wznawia ścieżkę wstrzymaną przez Pause().
     * @details Nie wznawia ścieżki na wstrzymanej szynie (Bus::Paused()) - ta ruszy razem
     * z szyną.
     * @return Ta sama muzyka.
     */
    Music& Resume();

    /** @brief Sprawdza, czy ścieżka jest wstrzymana - przez Pause() albo przez swoją szynę. */
    bool IsPaused() const;

    /**
     * @brief Przewija ścieżkę do podanego miejsca.
     * @details Przed startem odtworzenia (do końca klatki) miejsce zostaje zapamiętane. Gdy
     * ścieżka nie gra, nic nie robi.
     * @param seconds Czas od początku pliku w sekundach.
     * @return Ta sama muzyka.
     */
    Music& PlaybackTime(float seconds);

    /** @brief Podaje miejsce odtwarzania w sekundach (0, gdy ścieżka nie gra). */
    float PlaybackTime() const;

    /** @brief Podaje długość pliku w sekundach (0, gdy ścieżka nie gra albo jeszcze nie ruszyła). */
    float Duration() const;

    /**
     * @brief Ustawia zdarzenie końca ścieżki.
     * @details Przychodzi na końcu klatki, w której ścieżka bez pętli doszła do końca. Stop(),
     * zastąpienie inną ścieżką na szynie Exclusive(), zwolnienie sceny i zamknięcie silnika
     * nie wywołują zdarzenia. Ustawienie zostaje do następnego OnFinish().
     * @code{.cpp}
     * engine.Audio().Music("victory.ogg").Looped(false).OnFinish([](audio::Music&) {
     *     Engine::Get().Audio().Music("theme.ogg").Play();
     * });
     * @endcode
     * @param callback Reakcja na koniec.
     * @return Ta sama muzyka.
     */
    Music& OnFinish(std::function<void(Music&)> callback) {
        onFinish_ = std::move(callback);
        return *this;
    }

  private:
    friend class ::AudioMixer; // zapisuje głos ścieżki i wywołuje OnFinish()

    Voice voice_;                           /**< Odtworzenie ścieżki (martwe - nie gra). */
    std::function<void(Music&)> onFinish_;  /**< Zdarzenie końca ścieżki. */
};

} // namespace audio
