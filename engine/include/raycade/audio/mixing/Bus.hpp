/**
 * @file Bus.hpp
 * @brief Klasa audio::Bus - szyna miksera (grupa dźwięków ze wspólną głośnością).
 */
#pragma once

#include <string>
#include <utility>

class AudioMixer;

namespace audio {

/**
 * @brief Szyna miksera: grupa dźwięków ze wspólną głośnością, wyciszeniem i pauzą.
 * @details Szynę pobiera się po nazwie, jak dźwięk: `engine.Audio().Bus("efekty")`.
 * Pierwsze pobranie tworzy ją pod szyną główną, a `Audio().Bus()` bez nazwy to sama szyna
 * główna (jej ustawienia ma też sama fasada: `engine.Audio().Volume(0.8f)`). Szyny tworzą
 * drzewo (Parent()): głośność dźwięku jest mnożona przez głośności wszystkich szyn w górę
 * drzewa, a wyciszenie albo pauza rodzica dotyczy też dzieci.
 * @code{.cpp}
 * engine.Audio().Volume(0.8f);                          // szyna główna
 * engine.Audio().Bus("music").Volume(0.5f);              // szyna muzyki (Audio::kMusicBus)
 * engine.Audio().Bus("kroki").Parent("efekty").Volume(0.6f);
 * engine.Audio().Sound("step.wav").Bus("kroki");
 *
 * engine.Audio().Bus("efekty").Paused();                 // menu pauzy: efekty i kroki stoją
 * @endcode
 *
 * Szynę "music" (Audio::kMusicBus) silnik tworzy sam, z Exclusive() - na niej gra
 * audio::Music. Nazwa szyny to identyfikator, więc jest bez polskich znaków. Literówka w nazwie
 * tworzy nową szynę zamiast błędu - jej powstanie widać w logu (LogLevel::Debug).
 */
class Bus {
  public:
    /**
     * @brief Tworzy szynę (robi to Audio::Bus(), gra nie tworzy szyn sama).
     * @param mixer Mikser silnika.
     * @param name Nazwa szyny (pusta - szyna główna).
     */
    Bus(AudioMixer& mixer, std::string name) : mixer_(&mixer), name_(std::move(name)) {}

    Bus(const Bus&) = delete;
    Bus& operator=(const Bus&) = delete;

    /** @brief Podaje nazwę szyny (pusta - szyna główna). */
    const std::string& Name() const { return name_; }

    /**
     * @brief Podpina szynę pod inną szynę.
     * @details Szyna główna nie ma rodzica. Podpięcie, które utworzyłoby cykl (szyna pod
     * własnym dzieckiem), jest pomijane z wpisem w logu.
     * @param parent Nazwa rodzica; pusta - szyna główna. Nieistniejący rodzic powstaje.
     * @return Ta sama szyna.
     */
    Bus& Parent(std::string parent);

    /** @brief Podaje nazwę rodzica (pusta - szyna główna). */
    const std::string& Parent() const { return parent_; }

    /**
     * @brief Ustawia głośność szyny.
     * @param volume Mnożnik (0 - cisza, 1 - bez zmian).
     * @return Ta sama szyna.
     */
    Bus& Volume(float volume);

    /** @brief Podaje głośność szyny (bez rodziców). */
    float Volume() const { return volume_; }

    /**
     * @brief Wycisza szynę i jej dzieci; dźwięki grają dalej (bez straty pozycji).
     * @param value Czy wyciszyć.
     * @return Ta sama szyna.
     */
    Bus& Muted(bool value = true) {
        muted_ = value;
        return *this;
    }

    /** @brief Sprawdza, czy szyna jest wyciszona (bez rodziców). */
    bool IsMuted() const { return muted_; }

    /**
     * @brief Wstrzymuje dźwięki szyny i jej dzieci; zdjęcie flagi wznawia je od tego miejsca.
     * @details Play() na wstrzymanej szynie zgłasza odtworzenie, które ruszy po wznowieniu.
     * @param value Czy wstrzymać.
     * @return Ta sama szyna.
     */
    Bus& Paused(bool value = true) {
        paused_ = value;
        return *this;
    }

    /** @brief Sprawdza, czy szyna jest wstrzymana (bez rodziców). */
    bool IsPaused() const { return paused_; }

    /**
     * @brief Pozwala grać na szynie tylko jednemu dźwiękowi naraz.
     * @details Start nowego dźwięku wycisza pozostałe dźwięki tej szyny (ich
     * Source::FadeOutTime()) - przy muzyce z FadeInTime() daje to przenikanie ścieżek. Dotyczy
     * dźwięków samej szyny, nie jej dzieci. Tak samo działa szyna dialogów: nowa kwestia ucina
     * poprzednią.
     * @code{.cpp}
     * engine.Audio().Bus("dialogi").Exclusive();
     * engine.Audio().Sound("kwestia2.ogg").Bus("dialogi").Play(); // ucina poprzednią kwestię
     * @endcode
     * @param value Czy tylko jeden dźwięk naraz.
     * @return Ta sama szyna.
     */
    Bus& Exclusive(bool value = true) {
        exclusive_ = value;
        return *this;
    }

    /** @brief Sprawdza, czy na szynie gra tylko jeden dźwięk naraz. */
    bool IsExclusive() const { return exclusive_; }

    /**
     * @brief Ogranicza liczbę głosów naraz na szynie i jej dzieciach.
     * @details Play() ponad limit zatrzymuje najstarszy głos szyny. Limit szyny głównej to
     * rozmiar całej puli głosów (domyślnie 32, najwyżej 128).
     * @param count Największa liczba głosów; 0 - bez limitu (szyna główna: 128).
     * @return Ta sama szyna.
     */
    Bus& MaxVoices(int count);

    /** @brief Podaje limit głosów naraz (0 - bez limitu). */
    int MaxVoices() const { return maxVoices_; }

    /**
     * @brief Zatrzymuje wszystkie dźwięki szyny i jej dzieci (z ich Source::FadeOutTime()).
     * @return Ta sama szyna.
     */
    Bus& Stop();

    /** @brief Podaje liczbę trwających odtworzeń na szynie i jej dzieciach. */
    int VoiceCount() const;

  private:
    friend class ::AudioMixer; // czyta stan szyn przy miksowaniu

    AudioMixer* mixer_;      /**< Mikser silnika. */
    std::string name_;       /**< Nazwa; pusta - szyna główna. */
    std::string parent_;     /**< Rodzic; pusty - szyna główna. */
    float volume_ = 1.0f;    /**< Głośność. */
    bool muted_ = false;     /**< Wyciszenie. */
    bool paused_ = false;    /**< Pauza. */
    bool exclusive_ = false; /**< Jeden dźwięk naraz. */
    int maxVoices_ = 0;      /**< Limit głosów; 0 - bez limitu. */
};

} // namespace audio
