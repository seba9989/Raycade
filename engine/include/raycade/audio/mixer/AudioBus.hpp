/**
 * @file AudioBus.hpp
 * @brief Klasa AudioBus - szyna miksera (grupa dźwięków ze wspólną głośnością).
 */
#pragma once

#include <string>
#include <utility>

class AudioMixer;

/**
 * @brief Szyna miksera: grupa dźwięków ze wspólną głośnością, wyciszeniem i pauzą.
 * @details Szynę pobiera się z silnika po nazwie, jak dźwięk: Engine::AudioBus("muzyka").
 * Pierwsze pobranie tworzy ją pod szyną główną, a Engine::AudioBus() bez nazwy to sama szyna
 * główna. Szyny tworzą drzewo (Parent()): głośność dźwięku jest mnożona przez głośności
 * wszystkich szyn w górę drzewa, a wyciszenie albo pauza rodzica dotyczy też dzieci.
 * @code{.cpp}
 * engine.AudioBus().Volume(0.8f);                       // szyna główna
 * engine.AudioBus("muzyka").Volume(0.5f);
 * engine.AudioBus("kroki").Parent("efekty").Volume(0.6f);
 * engine.Audio("step.wav").Bus("kroki");
 *
 * engine.AudioBus("efekty").Paused();                   // menu pauzy: efekty i kroki stoją
 * @endcode
 *
 * Nazwa szyny to identyfikator, więc jest bez polskich znaków. Literówka w nazwie tworzy nową
 * szynę zamiast błędu - jej powstanie widać w logu (LogLevel::Debug).
 */
class AudioBus {
  public:
    /**
     * @brief Tworzy szynę (robi to Engine::AudioBus(), gra nie tworzy szyn sama).
     * @param mixer Mikser silnika.
     * @param name Nazwa szyny (pusta - szyna główna).
     */
    AudioBus(AudioMixer& mixer, std::string name) : mixer_(&mixer), name_(std::move(name)) {}

    AudioBus(const AudioBus&) = delete;
    AudioBus& operator=(const AudioBus&) = delete;

    /** @brief Podaje nazwę szyny (pusta - szyna główna). */
    const std::string& Name() const { return name_; }

    /**
     * @brief Podpina szynę pod inną szynę.
     * @details Szyna główna nie ma rodzica. Podpięcie, które utworzyłoby cykl (szyna pod
     * własnym dzieckiem), jest pomijane z wpisem w logu.
     * @param parent Nazwa rodzica; pusta - szyna główna. Nieistniejący rodzic powstaje.
     * @return Ta sama szyna.
     */
    AudioBus& Parent(std::string parent);

    /** @brief Podaje nazwę rodzica (pusta - szyna główna). */
    const std::string& Parent() const { return parent_; }

    /**
     * @brief Ustawia głośność szyny.
     * @param volume Mnożnik (0 - cisza, 1 - bez zmian).
     * @return Ta sama szyna.
     */
    AudioBus& Volume(float volume);

    /** @brief Podaje głośność szyny (bez rodziców). */
    float Volume() const { return volume_; }

    /**
     * @brief Wycisza szynę i jej dzieci; dźwięki grają dalej (bez straty pozycji).
     * @param value Czy wyciszyć.
     * @return Ta sama szyna.
     */
    AudioBus& Muted(bool value = true) {
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
    AudioBus& Paused(bool value = true) {
        paused_ = value;
        return *this;
    }

    /** @brief Sprawdza, czy szyna jest wstrzymana (bez rodziców). */
    bool IsPaused() const { return paused_; }

    /**
     * @brief Pozwala grać na szynie tylko jednemu dźwiękowi naraz.
     * @details Start nowego dźwięku wycisza pozostałe dźwięki tej szyny (ich
     * Audio::FadeOutTime()) - przy muzyce z FadeInTime() daje to przenikanie ścieżek. Dotyczy
     * dźwięków samej szyny, nie jej dzieci. Tak samo działa szyna dialogów: nowa kwestia ucina
     * poprzednią.
     * @code{.cpp}
     * engine.AudioBus("muzyka").Exclusive();
     * engine.Audio("boss.ogg").Play(); // theme.ogg wycisza się, boss.ogg narasta
     * @endcode
     * @param value Czy tylko jeden dźwięk naraz.
     * @return Ta sama szyna.
     */
    AudioBus& Exclusive(bool value = true) {
        exclusive_ = value;
        return *this;
    }

    /** @brief Sprawdza, czy na szynie gra tylko jeden dźwięk naraz. */
    bool IsExclusive() const { return exclusive_; }

    /**
     * @brief Zatrzymuje wszystkie dźwięki szyny i jej dzieci (z ich Audio::FadeOutTime()).
     * @return Ta sama szyna.
     */
    AudioBus& Stop();

    /** @brief Podaje liczbę trwających odtworzeń na szynie i jej dzieciach. */
    int VoiceCount() const;

  private:
    friend class AudioMixer; // czyta stan szyn przy miksowaniu

    AudioMixer* mixer_;    /**< Mikser silnika. */
    std::string name_;     /**< Nazwa; pusta - szyna główna. */
    std::string parent_;   /**< Rodzic; pusty - szyna główna. */
    float volume_ = 1.0f;  /**< Głośność. */
    bool muted_ = false;   /**< Wyciszenie. */
    bool paused_ = false;  /**< Pauza. */
    bool exclusive_ = false; /**< Jeden dźwięk naraz. */
};
