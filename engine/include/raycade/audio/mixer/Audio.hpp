/**
 * @file Audio.hpp
 * @brief Klasa Audio - definicja dźwięku (pliki i ustawienia odtwarzania).
 */
#pragma once

#include <string>
#include <utility>
#include <vector>

#include <raycade/audio/mixer/Voice.hpp>

class AudioMixer;

/**
 * @brief Dźwięk: pliki i ustawienia, z którymi gra każde jego odtworzenie.
 * @details Dźwięk pobiera się z silnika po nazwie, jak scenę: Engine::Audio("coin.wav").
 * Pierwsze pobranie tworzy go z ustawieniami domyślnymi, kolejne zwracają ten sam obiekt, więc
 * konfiguruje się go raz (np. w main()), a odtwarza w dowolnym miejscu gry:
 * @code{.cpp}
 * engine.Audio("coin.wav").Volume(0.7f).PitchVariation(0.08f).Bus("efekty");
 * // ...
 * engine.Audio("coin.wav").Play();
 * @endcode
 *
 * Nazwa jest zarazem plikiem (względem katalogu assets/, jak w Assets), chyba że Files() poda
 * listę plików - wtedy nazwa jest tylko identyfikatorem, a każde Play() losuje jeden plik:
 * @code{.cpp}
 * engine.Audio("kroki").Files({"step1.wav", "step2.wav", "step3.wav"}).Bus("efekty");
 * @endcode
 *
 * Pliki wczytują się przy pierwszym odtworzeniu. Plik, którego nie da się wczytać, gra ciszą,
 * a w logu pojawia się jeden wpis na plik. Ustawienia zostają w silniku do końca programu,
 * a dane próbek są zwalniane razem z nieużywanymi zasobami (po zmianie scen).
 *
 * Ustawienia dźwięku mnożą się z ustawieniami jego szyny (AudioBus) i pojedynczego
 * odtworzenia (Voice).
 */
class Audio {
  public:
    /**
     * @brief Tworzy dźwięk (robi to Engine::Audio(), gra nie tworzy dźwięków sama).
     * @param mixer Mikser silnika.
     * @param name Nazwa dźwięku.
     */
    Audio(AudioMixer& mixer, std::string name) : mixer_(&mixer), name_(std::move(name)) {}

    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    /** @brief Podaje nazwę dźwięku (tę z Engine::Audio()). */
    const std::string& Name() const { return name_; }

    /**
     * @brief Ustawia listę plików; każde Play() losuje jeden z nich.
     * @details Ten sam plik nie wypada dwa razy z rzędu (gdy plików jest więcej niż jeden).
     * Pusta lista oznacza, że plikiem jest nazwa dźwięku. Ponowne wywołanie zastępuje listę.
     * @param files Nazwy plików względem katalogu assets/.
     * @return Ten sam dźwięk.
     */
    Audio& Files(std::vector<std::string> files);

    /** @brief Podaje listę plików (pusta - plikiem jest nazwa). */
    const std::vector<std::string>& Files() const { return files_; }

    /**
     * @brief Ustawia głośność dźwięku.
     * @param volume Mnożnik (0 - cisza, 1 - głośność pliku).
     * @return Ten sam dźwięk.
     */
    Audio& Volume(float volume);

    /** @brief Podaje głośność dźwięku. */
    float Volume() const { return volume_; }

    /**
     * @brief Ustawia wysokość dźwięku; razem z nią zmienia się tempo.
     * @param pitch Mnożnik (0.5 - oktawa niżej i dwa razy wolniej).
     * @return Ten sam dźwięk.
     */
    Audio& Pitch(float pitch);

    /** @brief Podaje wysokość dźwięku. */
    float Pitch() const { return pitch_; }

    /**
     * @brief Ustawia losową zmianę wysokości przy każdym odtworzeniu.
     * @details Powtarzany dźwięk (kroki, strzały, monety) z odrobiną losowości nie brzmi jak
     * ta sama próbka puszczana w kółko.
     * @param variation Największe odchylenie, np. 0.08 - wysokość od 0.92 do 1.08 Pitch().
     * @return Ten sam dźwięk.
     */
    Audio& PitchVariation(float variation);

    /** @brief Podaje losową zmianę wysokości. */
    float PitchVariation() const { return pitchVariation_; }

    /**
     * @brief Ustawia panoramę dźwięku.
     * @param pan Od -1 (lewo) przez 0 (środek) do 1 (prawo).
     * @return Ten sam dźwięk.
     */
    Audio& Pan(float pan);

    /** @brief Podaje panoramę dźwięku. */
    float Pan() const { return pan_; }

    /**
     * @brief Ustawia szynę, na której gra dźwięk.
     * @details Szyna, której jeszcze nie ma, powstaje pod szyną główną (patrz Engine::AudioBus()).
     * Zmiana działa od następnego Play() - trwające odtworzenia zostają na starej szynie.
     * @param bus Nazwa szyny; pusta - szyna główna.
     * @return Ten sam dźwięk.
     */
    Audio& Bus(std::string bus);

    /** @brief Podaje nazwę szyny (pusta - szyna główna). */
    const std::string& Bus() const { return bus_; }

    /**
     * @brief Odtwarza dźwięk.
     * @details Odtworzenie startuje na końcu klatki, więc ustawienia podane na zwróconym
     * uchwycie działają od pierwszej próbki. Bez urządzenia audio zwraca martwy uchwyt.
     * @return Uchwyt odtworzenia.
     */
    Voice Play();

    /**
     * @brief Zatrzymuje wszystkie odtworzenia tego dźwięku.
     * @return Ten sam dźwięk.
     */
    Audio& Stop();

    /** @brief Sprawdza, czy gra choć jedno odtworzenie tego dźwięku. */
    bool IsPlaying() const { return VoiceCount() > 0; }

    /** @brief Podaje liczbę trwających odtworzeń tego dźwięku. */
    int VoiceCount() const;

  private:
    friend class AudioMixer; // czyta ustawienia przy starcie głosu i losuje pliki

    AudioMixer* mixer_;               /**< Mikser silnika. */
    std::string name_;                /**< Nazwa (i domyślny plik). */
    std::vector<std::string> files_;  /**< Warianty; puste - plikiem jest nazwa. */
    std::string bus_;                 /**< Szyna; pusta - główna. */
    float volume_ = 1.0f;             /**< Głośność. */
    float pitch_ = 1.0f;              /**< Wysokość. */
    float pitchVariation_ = 0.0f;     /**< Losowe odchylenie wysokości. */
    float pan_ = 0.0f;                /**< Panorama. */
    int lastFile_ = -1;               /**< Ostatnio wylosowany plik (bez powtórzeń). */
};
