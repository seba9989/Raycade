/**
 * @file AudioMixer.hpp
 * @brief Klasa AudioMixer - prywatny mikser silnika (urządzenie, pula głosów, dźwięki, szyny).
 * @details Nagłówek prywatny: gra korzysta z Engine::Audio(), Engine::AudioBus() i Voice.
 */
#pragma once

#include <raylib.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <raycade/audio/mixer/Audio.hpp>
#include <raycade/audio/mixer/AudioBus.hpp>
#include <raycade/audio/mixer/Voice.hpp>
#include <raycade/engine/EngineTypes.hpp>

/**
 * @brief Mikser: urządzenie audio, dźwięki i szyny według nazw oraz pula głosów.
 * @details Jedyny obiekt tej klasy należy do Engine, który otwiera urządzenie w Init(), woła
 * Update() na końcu każdej klatki i zamyka urządzenie w Shutdown(). Uchwyty Voice odnajdują
 * mikser przez Current().
 *
 * Krótki dźwięk jest wczytywany do pamięci raz na plik (Clip), a każdy głos gra własny alias
 * raylib (LoadSoundAlias) - tak ten sam plik może grać kilka razy naraz. Dźwięk z
 * Audio::Streamed() gra jako strumień raylib (Music) czytany z pliku, a zapętlony bez Streamed()
 * - jako strumień z bajtów pliku w pamięci, bo tylko strumień raylib zapętla się bez przerwy.
 */
class AudioMixer {
  public:
    /** @brief Zapis w logu silnika. */
    using LogFunction = std::function<void(LogLevel, const std::string&)>;
    /** @brief Zamiana nazwy pliku na ścieżkę (Assets::Path()). */
    using PathFunction = std::function<std::string(const std::string&)>;

    /**
     * @brief Tworzy mikser bez urządzenia (dźwięki i szyny można konfigurować przed Open()).
     * @param log Zapis w logu.
     * @param path Zamiana nazwy pliku na ścieżkę.
     */
    AudioMixer(LogFunction log, PathFunction path);

    /** @brief Zamyka urządzenie (Close()). */
    ~AudioMixer();

    AudioMixer(const AudioMixer&) = delete;
    AudioMixer& operator=(const AudioMixer&) = delete;

    /** @brief Podaje mikser silnika (nullptr przed jego utworzeniem i po zniszczeniu). */
    static AudioMixer* Current() { return current_; }

    /** @brief Otwiera urządzenie audio; błąd zapisuje w logu. */
    void Open();

    /** @brief Zatrzymuje wszystkie głosy, zwalnia dane i zamyka urządzenie. */
    void Close();

    /** @brief Sprawdza, czy urządzenie audio jest otwarte. */
    bool IsReady() const { return ready_; }

    /** @brief Podaje dźwięk o nazwie, tworząc go przy pierwszym użyciu. */
    ::Audio& Sound(const std::string& name);

    /** @brief Podaje szynę o nazwie (pusta - główna), tworząc ją przy pierwszym użyciu. */
    ::AudioBus& Bus(const std::string& name);

    /**
     * @brief Uruchamia zgłoszone głosy, aktualizuje parametry trwających i usuwa skończone.
     * @param dt Czas klatki w sekundach.
     */
    void Update(float dt);

    /**
     * @brief Zwalnia dane plików, których nie gra żaden głos.
     * @return Liczba zwolnionych plików.
     */
    std::size_t Purge();

    /** @name Dla Audio, AudioBus i Voice */
    /** @{ */
    /** @brief Zgłasza odtworzenie dźwięku. */
    Voice Play(::Audio& audio);
    /** @brief Zatrzymuje głosy dźwięku. */
    void Stop(const ::Audio& audio);
    /** @brief Zatrzymuje głosy szyny i jej dzieci. */
    void Stop(const ::AudioBus& bus);
    /** @brief Liczy głosy dźwięku. */
    int VoiceCount(const ::Audio& audio) const;
    /** @brief Liczy głosy szyny i jej dzieci. */
    int VoiceCount(const ::AudioBus& bus) const;
    /** @brief Podpina szynę pod rodzica, jeśli nie powstanie cykl. */
    void Reparent(::AudioBus& bus, const std::string& parent);
    /** @brief Poprawia limit głosów szyny (szyna główna: od 1 do rozmiaru puli). */
    int ClampVoices(const ::AudioBus& bus, int count) const;
    /** @} */

    /** @brief Głos w puli. */
    struct Slot {
        std::uint32_t generation = 1; /**< Generacja (rośnie przy każdym zwolnieniu). */
        bool used = false;            /**< Czy miejsce jest zajęte. */
        bool started = false;         /**< Czy głos już gra (false - czeka na koniec klatki). */
        bool paused = false;          /**< Czy dźwięk jest wstrzymany w raylib. */
        ::Audio* audio = nullptr;     /**< Dźwięk, z którego powstał głos. */
        std::string bus;              /**< Szyna (ustalona przy Play()). */
        std::string path;             /**< Wylosowany plik. */
        bool stream = false;          /**< Czy głos gra strumień (music), a nie alias (sound). */
        ::Sound sound{};              /**< Alias próbek pliku. */
        ::Music music{};              /**< Strumień (Streamed() albo Looped()). */
        std::shared_ptr<const std::vector<unsigned char>> bytes; /**< Plik dla strumienia z pamięci. */
        float fade = 1.0f;            /**< Bieżący poziom narastania/wyciszania (0-1). */
        float fadeRate = 0.0f;        /**< Zmiana fade na sekundę. */
        bool stopping = false;        /**< Czy głos wycisza się po Stop() (potem zwolnienie). */
        bool userPaused = false;      /**< Czy wstrzymany przez Voice::Pause(). */
        float time = 0.0f;            /**< Czas odtwarzania liczony przez silnik (alias). */
        float seek = -1.0f;           /**< Przewinięcie do wykonania po starcie (< 0 - brak). */
        std::uint64_t order = 0;      /**< Numer Play() - najstarszy głos ma najmniejszy. */
        std::function<void(Voice&)> onFinish; /**< Zdarzenie końca (Voice::OnFinish()). */
        float volume = 1.0f;          /**< Głośność głosu (Voice::Volume()). */
        float pitch = 1.0f;           /**< Wysokość głosu (Voice::Pitch()). */
        float pan = 0.0f;             /**< Panorama głosu (Voice::Pan()). */
        float variation = 1.0f;       /**< Wylosowana zmiana wysokości (PitchVariation()). */
    };

    /** @brief Podaje głos uchwytu albo nullptr dla martwego uchwytu. */
    Slot* Find(const Voice& voice);
    /** @copydoc Find() */
    const Slot* Find(const Voice& voice) const;

    /** @brief Zatrzymuje głos. */
    void Stop(Slot& slot);
    /** @brief Przewija głos (strumień) albo zapisuje w logu, że się nie da. */
    void Seek(Slot& slot, float seconds);
    /** @brief Podaje miejsce odtwarzania głosu w sekundach. */
    float PlaybackTime(const Slot& slot) const;
    /** @brief Podaje długość pliku głosu w sekundach (0 przed startem). */
    float Duration(const Slot& slot) const;

  private:
    /** @brief Dane jednego pliku w pamięci. */
    struct Clip {
        ::Sound sound{}; /**< Próbki (źródło aliasów). */
    };

    /** @brief Stan szyny razem z rodzicami, liczony raz na klatkę. */
    struct BusState {
        float gain = 1.0f;   /**< Iloczyn głośności (0 przy wyciszeniu). */
        bool paused = false; /**< Czy szyna albo rodzic jest wstrzymany. */
    };

    /** @brief Wczytuje plik do pamięci (raz); nullptr przy błędzie (raz w logu). */
    const Clip* LoadClip(const std::string& path);
    /** @brief Wczytuje bajty pliku (raz) dla strumienia z pamięci; nullptr przy błędzie. */
    std::shared_ptr<const std::vector<unsigned char>> LoadBytes(const std::string& path);
    /** @brief Zapisuje w logu błąd wczytania (raz na plik). */
    void Failed(const std::string& path);
    /** @brief Wycisza pozostałe głosy szyny Exclusive() po starcie głosu slot. */
    void Replace(const Slot& slot);
    /** @brief Uruchamia zgłoszony głos; false - nie udało się wczytać pliku. */
    bool Start(Slot& slot);
    /**
     * @brief Zwalnia miejsce głosu.
     * @param slot Głos.
     * @param notify Czy wywołać zdarzenie końca (na końcu klatki).
     */
    void Release(Slot& slot, bool notify = true);
    /** @brief Zwalnia najstarszy głos spełniający warunek; false - nie ma takiego. */
    bool StealOldest(const std::function<bool(const Slot&)>& match);
    /** @brief Robi miejsce dla nowego głosu dźwięku (limity dźwięku, szyn i puli). */
    void MakeRoom(const ::Audio& audio);
    /** @brief Wywołuje zdarzenia końca zebrane w tej klatce. */
    void Notify();
    /** @brief Ustawia głośność, wysokość i panoramę głosu w raylib. */
    void Apply(Slot& slot, const BusState& bus);
    /** @brief Liczy stan szyny razem z rodzicami. */
    BusState StateOf(const std::string& bus) const;
    /** @brief Sprawdza, czy szyna jest szyną ancestor albo jej potomkiem. */
    bool IsWithin(const std::string& bus, const std::string& ancestor) const;
    /** @brief Wybiera plik dla nowego głosu. */
    std::string PickFile(::Audio& audio);
    /** @brief Uchwyt miejsca. */
    Voice Handle(std::size_t index) const;

    static inline AudioMixer* current_ = nullptr; /**< Mikser silnika. */

    LogFunction log_;                             /**< Zapis w logu. */
    PathFunction path_;                           /**< Nazwa pliku -> ścieżka. */
    bool ready_ = false;                          /**< Czy urządzenie jest otwarte. */
    bool warnedNoDevice_ = false;                 /**< Czy zapisano brak urządzenia. */
    std::map<std::string, ::Audio> sounds_;       /**< Dźwięki według nazw. */
    std::map<std::string, ::AudioBus> buses_;     /**< Szyny według nazw ("" - główna). */
    std::unordered_map<std::string, Clip> clips_; /**< Pliki w pamięci według ścieżek. */
    /** Bajty plików dla zapętlonych strumieni z pamięci, według ścieżek. */
    std::unordered_map<std::string, std::shared_ptr<const std::vector<unsigned char>>> bytes_;
    std::set<std::string> failed_;                /**< Pliki, których nie wczytano. */
    std::vector<Slot> slots_;                     /**< Pula głosów (stały rozmiar). */
    std::uint64_t order_ = 0;                     /**< Licznik Play(). */
    bool warnedSeek_ = false;                     /**< Czy zapisano brak przewijania aliasu. */
    /** Zdarzenia końca do wywołania na końcu klatki. */
    std::vector<std::pair<std::function<void(Voice&)>, Voice>> finished_;
};
