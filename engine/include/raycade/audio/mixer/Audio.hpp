/**
 * @file Audio.hpp
 * @brief Klasa Audio - dźwięk silnika: dźwięki, muzyka, szyny i szyna główna.
 */
#pragma once

#include <string>

#include <raycade/audio/mixer/Bus.hpp>
#include <raycade/audio/mixer/Music.hpp>
#include <raycade/audio/mixer/Sound.hpp>
#include <raycade/audio/mixer/Voice.hpp>

class AudioMixer;

/**
 * @brief Dźwięk silnika: dostęp do dźwięków, muzyki i szyn miksera po nazwie.
 * @details Jedyny obiekt tej klasy podaje Engine::Audio(), tak jak Engine::Assets() podaje
 * zasoby. Dźwięki (audio::Sound), muzykę (audio::Music) i szyny (audio::Bus) pobiera się po
 * nazwie: pierwsze pobranie tworzy obiekt z ustawieniami domyślnymi, kolejne zwracają ten sam,
 * więc konfiguruje się je raz (np. w main(), także przed Init()), a używa w dowolnym miejscu.
 * Nieznana nazwa tworzy obiekt zamiast rzucać wyjątek (jak Assets - brakujący plik gra ciszą).
 * @code{.cpp}
 * engine.Audio().Volume(0.8f);                                  // szyna główna
 * engine.Audio().Bus("efekty").MaxVoices(24);
 * engine.Audio().Sound("coin.wav").Volume(0.7f).Bus("efekty");
 * engine.Audio().Music("theme.ogg").FadeInTime(1.5f).FadeOutTime(1.0f);
 * // ...
 * engine.Audio().Sound("coin.wav").Play();
 * engine.Audio().Music("theme.ogg").Play();
 * @endcode
 *
 * Ustawienia szyny głównej są też na samym obiekcie (Volume(), Muted(), Paused(),
 * MaxVoices(), Stop()) - to skróty do Bus() bez nazwy. Urządzenie audio otwiera Engine::Init(),
 * a zamyka Engine::Shutdown(); bez urządzenia (np. brak karty dźwiękowej) Play() nic nie robi,
 * a w logu jest jeden wpis.
 */
class Audio {
  public:
    /** @brief Nazwa szyny muzyki, tworzonej przez silnik z Bus::Exclusive(). */
    static constexpr const char* kMusicBus = "music";

    /**
     * @brief Tworzy fasadę miksera (robi to silnik).
     * @param mixer Mikser silnika.
     */
    explicit Audio(AudioMixer& mixer) : mixer_(&mixer) {}

    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    /**
     * @brief Podaje dźwięk o nazwie, tworząc go przy pierwszym użyciu.
     * @details Nazwa jest zarazem plikiem w assets/, chyba że Source::Files() poda listę
     * wariantów. Patrz audio::Sound.
     * @param name Nazwa dźwięku (albo pliku).
     * @return Dźwięk.
     */
    audio::Sound& Sound(const std::string& name);

    /**
     * @brief Podaje muzykę o nazwie, tworząc ją przy pierwszym użyciu.
     * @details Nazwy muzyki są osobne od nazw dźwięków. Patrz audio::Music.
     * @param name Nazwa ścieżki (albo pliku).
     * @return Muzyka.
     */
    audio::Music& Music(const std::string& name);

    /**
     * @brief Podaje szynę o nazwie, tworząc ją przy pierwszym użyciu (pod szyną główną).
     * @details Nazwa szyny to identyfikator, a literówka tworzy nową szynę - jej powstanie
     * widać w logu (LogLevel::Debug). Patrz audio::Bus.
     * @param name Nazwa szyny; pusta - szyna główna.
     * @return Szyna.
     */
    audio::Bus& Bus(const std::string& name = "");

    /** @brief Sprawdza, czy urządzenie audio jest otwarte. */
    bool IsReady() const;

    /**
     * @name Szyna główna
     * Skróty do Bus() bez nazwy.
     */
    /** @{ */

    /**
     * @brief Ustawia głośność całej gry.
     * @param volume Mnożnik (0 - cisza, 1 - bez zmian).
     * @return Ten sam obiekt.
     */
    Audio& Volume(float volume);

    /** @brief Podaje głośność całej gry. */
    float Volume() const;

    /**
     * @brief Wycisza całą grę; dźwięki grają dalej (bez straty pozycji).
     * @param value Czy wyciszyć.
     * @return Ten sam obiekt.
     */
    Audio& Muted(bool value = true);

    /** @brief Sprawdza, czy cała gra jest wyciszona. */
    bool IsMuted() const;

    /**
     * @brief Wstrzymuje wszystkie dźwięki; zdjęcie flagi wznawia je od tego miejsca.
     * @param value Czy wstrzymać.
     * @return Ten sam obiekt.
     */
    Audio& Paused(bool value = true);

    /** @brief Sprawdza, czy wszystkie dźwięki są wstrzymane. */
    bool IsPaused() const;

    /**
     * @brief Ustawia rozmiar puli głosów (ile odtworzeń gra naraz w całej grze).
     * @param count Liczba głosów (domyślnie 32, najwyżej 128); 0 - najwięcej.
     * @return Ten sam obiekt.
     */
    Audio& MaxVoices(int count);

    /** @brief Podaje rozmiar puli głosów. */
    int MaxVoices() const;

    /**
     * @brief Zatrzymuje wszystkie dźwięki i muzykę (z ich Source::FadeOutTime()).
     * @return Ten sam obiekt.
     */
    Audio& Stop();

    /** @brief Podaje liczbę trwających odtworzeń w całej grze. */
    int VoiceCount() const;

    /** @} */

  private:
    /** @brief Podaje szynę główną. */
    const audio::Bus& Master() const;

    AudioMixer* mixer_; /**< Mikser silnika. */
};
