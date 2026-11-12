/**
 * @file Voice.hpp
 * @brief Klasa Voice - uchwyt jednego odtworzenia dźwięku.
 */
#pragma once

#include <cstdint>

class Audio;

/**
 * @brief Uchwyt jednego odtworzenia dźwięku, zwracany przez Audio::Play().
 * @details Ustawienia z łańcucha działają od pierwszej próbki: Play() tylko zgłasza odtworzenie,
 * a silnik uruchamia je na końcu klatki. Ustawienia głosu mnożą się z ustawieniami dźwięku
 * (Audio) i szyn (AudioBus), np. głośność głosu 0.5 przy dźwięku 0.8 daje 0.4.
 * @code{.cpp}
 * Voice hit = engine.Audio("hit.wav").Play().Volume(0.5f).Pitch(1.2f);
 * if (hit.IsPlaying()) hit.Stop();
 * @endcode
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
     * @param pan Od -1 (lewo) przez 0 (środek) do 1 (prawo); dodawana do Audio::Pan().
     * @return Ten sam uchwyt.
     */
    Voice& Pan(float pan);

    /** @brief Podaje panoramę tego odtworzenia (0 dla martwego uchwytu). */
    float Pan() const;

    /**
     * @brief Zatrzymuje odtworzenie (z wyciszeniem Audio::FadeOutTime(), jeśli jest ustawione).
     * @return Ten sam uchwyt.
     */
    Voice& Stop();

    /** @brief Sprawdza, czy głos gra (istnieje, nie jest wstrzymany ani wyciszany po Stop()). */
    bool IsPlaying() const;

    /**
     * @brief Podaje dźwięk, z którego powstał głos.
     * @return Wskaźnik albo nullptr dla martwego uchwytu.
     */
    const ::Audio* Sound() const;

  private:
    friend class AudioMixer; // tworzy uchwyty

    /** @brief Tworzy uchwyt miejsca w puli. */
    Voice(std::uint32_t slot, std::uint32_t generation) : slot_(slot), generation_(generation) {}

    std::uint32_t slot_ = 0;       /**< Miejsce w puli głosów. */
    std::uint32_t generation_ = 0; /**< Generacja miejsca; 0 - uchwyt martwy. */
};
