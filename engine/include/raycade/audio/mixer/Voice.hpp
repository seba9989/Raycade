/**
 * @file Voice.hpp
 * @brief Klasa Voice - uchwyt jednego odtworzenia dźwięku.
 */
#pragma once

#include <cstdint>
#include <functional>

class AudioMixer;

namespace audio {

class Sound;

/**
 * @brief Uchwyt jednego odtworzenia dźwięku, zwracany przez Sound::Play().
 * @details Ustawienia z łańcucha działają od pierwszej próbki: Play() tylko zgłasza odtworzenie,
 * a silnik uruchamia je na końcu klatki. Ustawienia głosu mnożą się z ustawieniami dźwięku
 * (audio::Sound) i szyn (audio::Bus), np. głośność głosu 0.5 przy dźwięku 0.8 daje 0.4.
 * @code{.cpp}
 * audio::Voice hit = engine.Audio().Sound("hit.wav").Play().Volume(0.5f).Pitch(1.2f);
 * if (hit.IsPlaying()) hit.Stop();
 * @endcode
 *
 * Dźwięk w konkretnym miejscu świata gra przez audio::Emitter - głos nie ma położenia, żeby
 * nie dublować Transformable.
 *
 * Uchwyt jest lekki (numer miejsca w puli i numer generacji) i można go kopiować. Gdy głos się
 * skończy albo zostanie zatrzymany, uchwyt staje się martwy: settery i polecenia nic nie robią,
 * IsPlaying() zwraca false, a gettery liczbowe - wartości domyślne. To zwykła sytuacja, więc
 * nie trafia do logu. Domyślnie skonstruowany uchwyt jest martwy od początku.
 */
class Voice {
  public:
    /** @brief Tworzy martwy uchwyt (do późniejszego przypisania wyniku Play()). */
    Voice() = default;

    /**
     * @brief Ustawia głośność tego odtworzenia.
     * @param volume Mnożnik (0 - cisza, 1 - bez zmian).
     * @return Ten sam uchwyt.
     */
    Voice& Volume(float volume);

    /** @brief Podaje głośność tego odtworzenia (1 dla martwego uchwytu). */
    float Volume() const;

    /**
     * @brief Ustawia wysokość (i tempo) tego odtworzenia.
     * @param pitch Mnożnik (2 - oktawa wyżej i dwa razy szybciej).
     * @return Ten sam uchwyt.
     */
    Voice& Pitch(float pitch);

    /** @brief Podaje wysokość tego odtworzenia (1 dla martwego uchwytu). */
    float Pitch() const;

    /**
     * @brief Ustawia panoramę tego odtworzenia.
     * @param pan Od -1 (lewo) przez 0 (środek) do 1 (prawo); dodawana do Source::Pan().
     * @return Ten sam uchwyt.
     */
    Voice& Pan(float pan);

    /** @brief Podaje panoramę tego odtworzenia (0 dla martwego uchwytu). */
    float Pan() const;

    /**
     * @brief Wznawia odtworzenie wstrzymane przez Pause().
     * @details Nie wznawia głosu na wstrzymanej szynie (Bus::Paused()) - ten ruszy razem
     * z szyną.
     * @return Ten sam uchwyt.
     */
    Voice& Play();

    /**
     * @brief Wstrzymuje odtworzenie w bieżącym miejscu (wznawia je Play()).
     * @return Ten sam uchwyt.
     */
    Voice& Pause();

    /**
     * @brief Zatrzymuje odtworzenie (z wyciszeniem Source::FadeOutTime(), jeśli jest ustawione).
     * @return Ten sam uchwyt.
     */
    Voice& Stop();

    /** @brief Sprawdza, czy głos gra (istnieje, nie jest wstrzymany ani wyciszany po Stop()). */
    bool IsPlaying() const;

    /** @brief Sprawdza, czy głos jest wstrzymany - przez Pause() albo przez swoją szynę. */
    bool IsPaused() const;

    /**
     * @brief Przewija odtworzenie do podanego miejsca.
     * @details Działa dla dźwięków granych jako strumień (Sound::Streamed() albo
     * Source::Looped()); dla krótkiego dźwięku z pamięci jest pomijane z jednym wpisem w logu.
     * Przed startem głosu (do końca klatki) miejsce zostaje zapamiętane.
     * @param seconds Czas od początku pliku w sekundach.
     * @return Ten sam uchwyt.
     */
    Voice& PlaybackTime(float seconds);

    /**
     * @brief Podaje miejsce odtwarzania w sekundach od początku pliku.
     * @details Dla krótkiego dźwięku z pamięci to czas liczony przez silnik (z uwzględnieniem
     * wysokości), dla strumienia - pozycja podana przez raylib. 0 dla martwego uchwytu.
     */
    float PlaybackTime() const;

    /** @brief Podaje długość pliku w sekundach (0 przed startem głosu i dla martwego uchwytu). */
    float Duration() const;

    /**
     * @brief Ustawia zdarzenie końca odtworzenia.
     * @details Zdarzenie przychodzi na końcu klatki, w której głos się skończył albo zatrzymał
     * (po wyciszeniu). Uchwyt w argumencie jest już martwy - służy do porównania z zapamiętanym.
     * Głosy zatrzymane przez zwolnienie sceny i zamknięcie silnika nie wywołują zdarzenia.
     * @param callback Reakcja na koniec.
     * @return Ten sam uchwyt.
     */
    Voice& OnFinish(std::function<void(Voice&)> callback);

    /** @brief Porównuje uchwyty (ten sam głos). */
    bool operator==(const Voice&) const = default;

    /**
     * @brief Podaje dźwięk, z którego powstał głos.
     * @return Wskaźnik albo nullptr dla martwego uchwytu.
     */
    const audio::Sound* Sound() const;

  private:
    friend class ::AudioMixer; // tworzy uchwyty

    /** @brief Tworzy uchwyt miejsca w puli. */
    Voice(std::uint32_t slot, std::uint32_t generation) : slot_(slot), generation_(generation) {}

    std::uint32_t slot_ = 0;       /**< Miejsce w puli głosów. */
    std::uint32_t generation_ = 0; /**< Generacja miejsca; 0 - uchwyt martwy. */
};

} // namespace audio
