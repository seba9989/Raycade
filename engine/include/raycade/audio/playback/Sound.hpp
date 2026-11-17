/**
 * @file Sound.hpp
 * @brief Klasa audio::Sound - efekt dźwiękowy (wiele odtworzeń naraz).
 */
#pragma once

#include <algorithm>
#include <string>

#include <raycade/audio/playback/Source.hpp>
#include <raycade/audio/playback/Voice.hpp>

namespace audio {

/**
 * @brief Dźwięk: efekt, który może grać wiele razy naraz (monety, kroki, strzały, kliknięcia).
 * @details Dźwięk pobiera się po nazwie: `engine.Audio().Sound("coin.wav")`. Pierwsze pobranie
 * tworzy go z ustawieniami domyślnymi, kolejne zwracają ten sam obiekt, więc konfiguruje się go
 * raz (np. w main()), a odtwarza w dowolnym miejscu gry:
 * @code{.cpp}
 * engine.Audio().Sound("coin.wav").Volume(0.7f).PitchVariation(0.08f).Bus("efekty");
 * engine.Audio().Sound("kroki").Files({"step1.wav", "step2.wav", "step3.wav"}).Bus("efekty");
 * // ...
 * engine.Audio().Sound("coin.wav").Play();
 * engine.Audio().Sound("coin.wav").Play().Pitch(1.5f); // zmiana tylko tego odtworzenia
 * @endcode
 *
 * Każde Play() to nowe odtworzenie (audio::Voice). Odtworzenie należy do sceny, z której je
 * zagrano (patrz Global()). Muzykę - jedną ścieżkę naraz - gra audio::Music.
 *
 * W `using namespace audio;` nazwa Sound jest niejednoznaczna z typem raylib `::Sound` - stąd
 * w kodzie gry pełna nazwa `audio::Sound`.
 */
class Sound : public Source {
  public:
    /**
     * @brief Tworzy dźwięk (robi to Audio::Sound(), gra nie tworzy dźwięków sama).
     * @param mixer Mikser silnika.
     * @param name Nazwa dźwięku.
     */
    Sound(AudioMixer& mixer, std::string name) : Source(mixer, std::move(name), "") {}

    /**
     * @brief Ustawia losową zmianę wysokości przy każdym odtworzeniu.
     * @details Powtarzany dźwięk z odrobiną losowości nie brzmi jak ta sama próbka puszczana
     * w kółko.
     * @param variation Największe odchylenie, np. 0.08 - wysokość od 0.92 do 1.08 Pitch().
     * @return Ten sam dźwięk.
     */
    Sound& PitchVariation(float variation) {
        pitchVariation_ = std::clamp(variation, 0.0f, 0.99f);
        return *this;
    }

    /** @brief Podaje losową zmianę wysokości. */
    float PitchVariation() const { return pitchVariation_; }

    /**
     * @brief Ogranicza liczbę odtworzeń tego dźwięku naraz.
     * @details Play() ponad limit zatrzymuje najstarsze odtworzenie - dziesięć monet zebranych
     * naraz nie zagłuszy reszty gry.
     * @param count Największa liczba głosów; 0 - bez limitu.
     * @return Ten sam dźwięk.
     */
    Sound& MaxVoices(int count) {
        maxVoices_ = std::max(0, count);
        return *this;
    }

    /** @brief Podaje limit odtworzeń naraz (0 - bez limitu). */
    int MaxVoices() const { return maxVoices_; }

    /**
     * @brief Ustawia najkrótszy odstęp między odtworzeniami.
     * @details Play() wcześniej niż po tym czasie od poprzedniego zwraca martwy uchwyt - np.
     * dwadzieścia trafień w jednej klatce daje jeden dźwięk zamiast głośnego trzasku.
     * @param seconds Odstęp w sekundach; 0 - bez ograniczenia.
     * @return Ten sam dźwięk.
     */
    Sound& MinInterval(float seconds) {
        minInterval_ = std::max(0.0f, seconds);
        return *this;
    }

    /** @brief Podaje najkrótszy odstęp między odtworzeniami. */
    float MinInterval() const { return minInterval_; }

    /**
     * @brief Czyta plik na bieżąco w czasie odtwarzania zamiast wczytywać go do pamięci.
     * @details Do długich plików (dialogi): w pamięci jest tylko mały bufor. Każde odtworzenie
     * otwiera plik osobno, więc kilka odtworzeń naraz też działa.
     * @param value Czy czytać plik na bieżąco.
     * @return Ten sam dźwięk.
     */
    Sound& Streamed(bool value = true) {
        streamed_ = value;
        return *this;
    }

    /** @brief Sprawdza, czy plik jest czytany na bieżąco. */
    bool IsStreamed() const { return streamed_; }

    /**
     * @brief Odtwarza dźwięk.
     * @details Odtworzenie startuje na końcu klatki, więc ustawienia podane na zwróconym
     * uchwycie działają od pierwszej próbki. Bez urządzenia audio zwraca martwy uchwyt.
     * Na szynie Bus::Exclusive() odtworzenie zastępuje inne dźwięki tej szyny, a gdy ten sam
     * dźwięk już na niej gra, Play() zwraca jego trwające odtworzenie.
     * @return Uchwyt odtworzenia.
     */
    Voice Play();

  private:
    friend class ::AudioMixer; // MinInterval() liczy od ostatniego Play()

    float pitchVariation_ = 0.0f; /**< Losowe odchylenie wysokości. */
    int maxVoices_ = 0;           /**< Limit głosów naraz; 0 - bez limitu. */
    float minInterval_ = 0.0f;    /**< Najkrótszy odstęp między Play(). */
    double lastPlay_ = -1.0e9;    /**< Czas ostatniego Play() (GetTime()). */
};

} // namespace audio
