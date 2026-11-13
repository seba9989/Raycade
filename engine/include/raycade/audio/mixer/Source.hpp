/**
 * @file Source.hpp
 * @brief Klasa audio::Source - wspólna część dźwięku i muzyki (pliki i ustawienia).
 */
#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

class AudioMixer;

namespace audio {

/**
 * @brief Pliki i ustawienia wspólne dla dźwięku (audio::Sound) i muzyki (audio::Music).
 * @details Gra nie tworzy tej klasy sama - dostaje jej potomków z Engine::Audio():
 * `engine.Audio().Sound("coin.wav")` albo `engine.Audio().Music("theme.ogg")`. Settery zwracają
 * obiekt właściwego typu, więc łańcuch może mieszać ustawienia wspólne i własne:
 * @code{.cpp}
 * engine.Audio().Sound("coin.wav").Volume(0.7f).PitchVariation(0.08f).Bus("efekty");
 * @endcode
 *
 * Nazwa jest zarazem plikiem (względem katalogu assets/, jak w Assets), chyba że Files() poda
 * listę plików - wtedy nazwa jest tylko identyfikatorem, a każde Play() losuje jeden plik.
 *
 * Ustawienia mnożą się z ustawieniami szyny (audio::Bus) i pojedynczego odtworzenia
 * (audio::Voice) i zostają w silniku do końca programu. Dane plików to zwykłe zasoby
 * (Assets::Clip(), AudioClip): wczytują się przy pierwszym odtworzeniu, a plik, którego nie da
 * się wczytać, gra ciszą z jednym wpisem w logu.
 */
class Source {
  public:
    Source(const Source&) = delete;
    Source& operator=(const Source&) = delete;

    /** @brief Podaje nazwę (tę z Engine::Audio().Sound() albo Music()). */
    const std::string& Name() const { return name_; }

    /**
     * @brief Ustawia listę plików; każde Play() losuje jeden z nich.
     * @details Ten sam plik nie wypada dwa razy z rzędu (gdy plików jest więcej niż jeden).
     * Pusta lista oznacza, że plikiem jest nazwa. Ponowne wywołanie zastępuje listę.
     * @param files Nazwy plików względem katalogu assets/.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Files(this Self& self, std::vector<std::string> files) {
        self.files_ = std::move(files);
        self.lastFile_ = -1;
        return self;
    }

    /** @brief Podaje listę plików (pusta - plikiem jest nazwa). */
    const std::vector<std::string>& Files() const { return files_; }

    /**
     * @brief Ustawia głośność.
     * @param volume Mnożnik (0 - cisza, 1 - głośność pliku).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Volume(this Self& self, float volume) {
        self.volume_ = std::max(0.0f, volume);
        return self;
    }

    /** @brief Podaje głośność. */
    float Volume() const { return volume_; }

    /**
     * @brief Ustawia wysokość; razem z nią zmienia się tempo.
     * @param pitch Mnożnik (0.5 - oktawa niżej i dwa razy wolniej).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Pitch(this Self& self, float pitch) {
        self.pitch_ = std::max(0.01f, pitch);
        return self;
    }

    /** @brief Podaje wysokość. */
    float Pitch() const { return pitch_; }

    /**
     * @brief Ustawia panoramę.
     * @param pan Od -1 (lewo) przez 0 (środek) do 1 (prawo).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Pan(this Self& self, float pan) {
        self.pan_ = std::clamp(pan, -1.0f, 1.0f);
        return self;
    }

    /** @brief Podaje panoramę. */
    float Pan() const { return pan_; }

    /**
     * @brief Ustawia szynę, na której gra dźwięk.
     * @details Szyna, której jeszcze nie ma, powstaje pod szyną główną (patrz Audio::Bus()).
     * Zmiana działa od następnego Play() - trwające odtworzenia zostają na starej szynie.
     * @param bus Nazwa szyny; pusta - szyna główna.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Bus(this Self& self, std::string bus) {
        self.UseBus(std::move(bus));
        return self;
    }

    /** @brief Podaje nazwę szyny (pusta - szyna główna). */
    const std::string& Bus() const { return bus_; }

    /**
     * @brief Odtwarza w pętli, aż do Stop().
     * @details Pętla jest bez przerwy na styku (dźwięk gra jako strumień).
     * @param value Czy zapętlić.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Looped(this Self& self, bool value = true) {
        self.looped_ = value;
        return self;
    }

    /** @brief Sprawdza, czy dźwięk gra w pętli. */
    bool IsLooped() const { return looped_; }

    /**
     * @brief Odłącza odtworzenia od sceny - grają dalej po jej zwolnieniu.
     * @details Odtworzenie należy do sceny, z której je zagrano (z jej aktualizacji albo
     * konfiguracji), i cichnie przy UnloadScene()/ReloadScene() - jak węzeł dźwięku w drzewie
     * sceny. Global() działa jak dźwięk w obiekcie globalnym: przetrwa każdą zmianę scen,
     * niezależnie od tego, skąd go zagrano. Tak samo zachowują się odtworzenia z
     * Engine::OnUpdate() i z main(). Muzyka (audio::Music) jest globalna domyślnie.
     * @param value Czy grać niezależnie od scen.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Global(this Self& self, bool value = true) {
        self.global_ = value;
        return self;
    }

    /** @brief Sprawdza, czy odtworzenia grają niezależnie od scen. */
    bool IsGlobal() const { return global_; }

    /**
     * @brief Ustawia czas narastania głośności od zera na początku odtworzenia.
     * @param seconds Czas w sekundach; 0 - od razu pełna głośność.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& FadeInTime(this Self& self, float seconds) {
        self.fadeInTime_ = std::max(0.0f, seconds);
        return self;
    }

    /** @brief Podaje czas narastania głośności. */
    float FadeInTime() const { return fadeInTime_; }

    /**
     * @brief Ustawia czas wyciszania przy Stop() i przy zastąpieniu na szynie Exclusive().
     * @param seconds Czas w sekundach; 0 - zatrzymanie od razu.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& FadeOutTime(this Self& self, float seconds) {
        self.fadeOutTime_ = std::max(0.0f, seconds);
        return self;
    }

    /** @brief Podaje czas wyciszania. */
    float FadeOutTime() const { return fadeOutTime_; }

    /**
     * @brief Zatrzymuje wszystkie odtworzenia (z wyciszeniem FadeOutTime()).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Stop(this Self& self) {
        self.StopVoices();
        return self;
    }

    /**
     * @brief Sprawdza, czy wszystkie pliki dają się wczytać.
     * @details Sprawdza każdy plik z Files() (albo nazwę). Plik grany z pamięci jest przy tym
     * wczytywany do Assets (Assets::Clip()) - zostaje tam do najbliższego zwolnienia
     * nieużywanych zasobów. Dla strumienia (Sound::Streamed(), Music) sprawdza tylko, czy plik
     * istnieje. Nie wymaga urządzenia audio.
     * @return true, gdy każdy plik się wczytuje.
     */
    bool IsLoaded() const;

    /** @brief Sprawdza, czy gra choć jedno odtworzenie (wyciszane po Stop() się nie liczą). */
    bool IsPlaying() const { return VoiceCount() > 0; }

    /** @brief Podaje liczbę trwających odtworzeń (bez wyciszanych po Stop()). */
    int VoiceCount() const;

  protected:
    /**
     * @brief Tworzy dźwięk (robi to mikser silnika).
     * @param mixer Mikser silnika.
     * @param name Nazwa.
     * @param bus Szyna domyślna.
     */
    Source(AudioMixer& mixer, std::string name, std::string bus)
        : mixer_(&mixer), name_(std::move(name)), bus_(std::move(bus)) {}

    ~Source() = default;

    AudioMixer* mixer_;               /**< Mikser silnika. */
    std::string name_;                /**< Nazwa (i domyślny plik). */
    std::vector<std::string> files_;  /**< Warianty; puste - plikiem jest nazwa. */
    std::string bus_;                 /**< Szyna; pusta - główna. */
    float volume_ = 1.0f;             /**< Głośność. */
    float pitch_ = 1.0f;              /**< Wysokość. */
    float pan_ = 0.0f;                /**< Panorama. */
    float fadeInTime_ = 0.0f;         /**< Czas narastania. */
    float fadeOutTime_ = 0.0f;        /**< Czas wyciszania. */
    bool looped_ = false;             /**< Pętla. */
    bool streamed_ = false;           /**< Czytanie pliku na bieżąco. */
    bool global_ = false;             /**< Niezależność od scen. */
    int lastFile_ = -1;               /**< Ostatnio wylosowany plik (bez powtórzeń). */

  private:
    friend class ::AudioMixer; // czyta ustawienia przy starcie głosu i losuje pliki

    /** @brief Ustawia szynę (tworzy ją w mikserze). */
    void UseBus(std::string bus);
    /** @brief Zatrzymuje odtworzenia (Stop()). */
    void StopVoices();
};

} // namespace audio
