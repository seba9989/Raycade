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
 * Muzyka to zwykły dźwięk z Streamed() i Looped() na szynie z AudioBus::Exclusive() - nowa
 * ścieżka płynnie zastępuje poprzednią:
 * @code{.cpp}
 * engine.AudioBus("muzyka").Exclusive();
 * engine.Audio("theme.ogg").Streamed().Looped().Bus("muzyka").FadeInTime(1.5f).FadeOutTime(1.0f);
 * engine.Audio("theme.ogg").Play();
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
     * @brief Odtwarza dźwięk w pętli, aż do Stop().
     * @details Pętla jest bez przerwy na styku (dźwięk gra jako strumień z pamięci).
     * @param value Czy zapętlić.
     * @return Ten sam dźwięk.
     */
    Audio& Looped(bool value = true) {
        looped_ = value;
        return *this;
    }

    /** @brief Sprawdza, czy dźwięk gra w pętli. */
    bool IsLooped() const { return looped_; }

    /**
     * @brief Czyta plik na bieżąco w czasie odtwarzania zamiast wczytywać go do pamięci.
     * @details Do długich plików (muzyka, dialogi): w pamięci jest tylko mały bufor. Każde
     * odtworzenie otwiera plik osobno, więc kilka odtworzeń naraz też działa.
     * @param value Czy czytać plik na bieżąco.
     * @return Ten sam dźwięk.
     */
    Audio& Streamed(bool value = true) {
        streamed_ = value;
        return *this;
    }

    /** @brief Sprawdza, czy plik jest czytany na bieżąco. */
    bool IsStreamed() const { return streamed_; }

    /**
     * @brief Ustawia czas narastania głośności od zera na początku odtworzenia.
     * @param seconds Czas w sekundach; 0 - od razu pełna głośność.
     * @return Ten sam dźwięk.
     */
    Audio& FadeInTime(float seconds);

    /** @brief Podaje czas narastania głośności. */
    float FadeInTime() const { return fadeInTime_; }

    /**
     * @brief Ustawia czas wyciszania przy Stop() i przy zastąpieniu na szynie Exclusive().
     * @param seconds Czas w sekundach; 0 - zatrzymanie od razu.
     * @return Ten sam dźwięk.
     */
    Audio& FadeOutTime(float seconds);

    /** @brief Podaje czas wyciszania. */
    float FadeOutTime() const { return fadeOutTime_; }

    /**
     * @brief Ogranicza liczbę odtworzeń tego dźwięku naraz.
     * @details Play() ponad limit zatrzymuje najstarsze odtworzenie - dziesięć monet zebranych
     * naraz nie zagłuszy reszty gry.
     * @param count Największa liczba głosów; 0 - bez limitu.
     * @return Ten sam dźwięk.
     */
    Audio& MaxVoices(int count);

    /** @brief Podaje limit odtworzeń naraz (0 - bez limitu). */
    int MaxVoices() const { return maxVoices_; }

    /**
     * @brief Ustawia najkrótszy odstęp między odtworzeniami.
     * @details Play() wcześniej niż po tym czasie od poprzedniego zwraca martwy uchwyt - np.
     * dwadzieścia trafień w jednej klatce daje jeden dźwięk zamiast głośnego trzasku.
     * @param seconds Odstęp w sekundach; 0 - bez ograniczenia.
     * @return Ten sam dźwięk.
     */
    Audio& MinInterval(float seconds);

    /** @brief Podaje najkrótszy odstęp między odtworzeniami. */
    float MinInterval() const { return minInterval_; }

    /**
     * @brief Odtwarza dźwięk.
     * @details Odtworzenie startuje na końcu klatki, więc ustawienia podane na zwróconym
     * uchwycie działają od pierwszej próbki. Bez urządzenia audio zwraca martwy uchwyt.
     * Na szynie Exclusive() odtworzenie zastępuje inne dźwięki tej szyny, a gdy ten sam dźwięk
     * już na niej gra, Play() zwraca jego trwające odtworzenie zamiast zaczynać od nowa.
     * @return Uchwyt odtworzenia.
     */
    Voice Play();

    /**
     * @brief Zatrzymuje wszystkie odtworzenia tego dźwięku (z wyciszeniem FadeOutTime()).
     * @return Ten sam dźwięk.
     */
    Audio& Stop();

    /** @brief Sprawdza, czy gra choć jedno odtworzenie (wyciszane po Stop() się nie liczą). */
    bool IsPlaying() const { return VoiceCount() > 0; }

    /** @brief Podaje liczbę trwających odtworzeń (bez wyciszanych po Stop()). */
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
    float fadeInTime_ = 0.0f;         /**< Czas narastania. */
    float fadeOutTime_ = 0.0f;        /**< Czas wyciszania. */
    bool looped_ = false;             /**< Pętla. */
    bool streamed_ = false;           /**< Czytanie pliku na bieżąco. */
    int lastFile_ = -1;               /**< Ostatnio wylosowany plik (bez powtórzeń). */
    int maxVoices_ = 0;               /**< Limit głosów naraz; 0 - bez limitu. */
    float minInterval_ = 0.0f;        /**< Najkrótszy odstęp między Play(). */
    double lastPlay_ = -1.0e9;        /**< Czas ostatniego Play() (GetTime()). */
};
