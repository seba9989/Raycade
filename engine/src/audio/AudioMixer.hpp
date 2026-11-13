/**
 * @file AudioMixer.hpp
 * @brief Klasa AudioMixer - prywatny mikser silnika (urządzenie, pula głosów, dźwięki, szyny).
 * @details Nagłówek prywatny: gra korzysta z Engine::Audio() (fasada Audio) i audio::Voice.
 */
#pragma once

#include <raylib.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <raycade/assets/sound/AudioClip.hpp>
#include <raycade/audio/Audio.hpp>
#include <raycade/audio/mixing/Bus.hpp>
#include <raycade/audio/playback/Music.hpp>
#include <raycade/audio/playback/Sound.hpp>
#include <raycade/audio/playback/Source.hpp>
#include <raycade/audio/playback/Voice.hpp>
#include <raycade/core/EngineTypes.hpp>

class Assets;

/**
 * @brief Mikser: urządzenie audio, dźwięki, muzyka i szyny według nazw oraz pula głosów.
 * @details Jedyny obiekt tej klasy należy do Engine, który otwiera urządzenie w Init(), woła
 * Update() na końcu każdej klatki i zamyka urządzenie w Shutdown(). Uchwyty audio::Voice odnajdują
 * mikser przez Current().
 *
 * Dane plików pochodzą z Assets::Clip() (AudioClip, raz na plik), a każdy głos trzyma swój
 * klip, dopóki gra - dlatego Assets::Purge() nie zwalnia grającego pliku. Krótki dźwięk gra
 * jako własny alias raylib (LoadSoundAlias) - tak ten sam plik może grać kilka razy naraz.
 * Zapętlony bez Streamed() gra jako strumień z bajtów klipu, bo tylko strumień raylib zapętla
 * się bez przerwy. Dźwięk z Sound::Streamed() i muzyka (audio::Music) grają jako strumień
 * czytany z pliku przez każdy głos osobno - nie przechodzi przez Assets, ale błąd zgłasza tak
 * samo (Assets::Report()).
 */
class AudioMixer {
  public:
    /** @brief Zapis w logu silnika. */
    using LogFunction = std::function<void(LogLevel, const std::string&)>;

    /**
     * @brief Tworzy mikser bez urządzenia (dźwięki i szyny można konfigurować przed Open()).
     * @param log Zapis w logu.
     * @param assets Źródło danych plików (Assets silnika); musi żyć dłużej niż mikser.
     */
    AudioMixer(LogFunction log, ::Assets& assets);

    /** @brief Zatrzymuje głosy i zamyka urządzenie (StopAll(), CloseDevice()). */
    ~AudioMixer();

    AudioMixer(const AudioMixer&) = delete;
    AudioMixer& operator=(const AudioMixer&) = delete;

    /** @brief Podaje mikser silnika (nullptr przed jego utworzeniem i po zniszczeniu). */
    static AudioMixer* Current() { return current_; }

    /** @brief Otwiera urządzenie audio; błąd zapisuje w logu. */
    void Open();

    /**
     * @brief Zatrzymuje od razu wszystkie głosy (bez zdarzeń końca) i oddaje ich klipy.
     * @details Engine woła ją przed Assets::Clear(), a CloseDevice() po nim - klipy zwalniają
     * próbki, póki urządzenie jest otwarte.
     */
    void StopAll();

    /** @brief Zamyka urządzenie audio. */
    void CloseDevice();

    /** @brief Sprawdza, czy urządzenie audio jest otwarte. */
    bool IsReady() const { return ready_; }

    /** @brief Podaje fasadę dla gry (Engine::Audio()). */
    ::Audio& Facade() { return facade_; }

    /** @brief Podaje dźwięk o nazwie, tworząc go przy pierwszym użyciu. */
    audio::Sound& Sound(const std::string& name);

    /** @brief Podaje muzykę o nazwie, tworząc ją przy pierwszym użyciu. */
    audio::Music& Music(const std::string& name);

    /** @brief Podaje szynę o nazwie (pusta - główna), tworząc ją przy pierwszym użyciu. */
    audio::Bus& Bus(const std::string& name);

    /**
     * @brief Uruchamia zgłoszone głosy, aktualizuje parametry trwających i usuwa skończone.
     * @param dt Czas klatki w sekundach.
     */
    void Update(float dt);

    /**
     * @brief Sprawdza, czy wszystkie pliki dźwięku dają się wczytać (Source::IsLoaded()).
     * @param source Dźwięk albo muzyka.
     * @return true, gdy każdy plik z Files() (albo nazwa) się wczytuje.
     */
    bool IsLoaded(const audio::Source& source) const;

    /** @name Dla audio::Sound, audio::Music, audio::Bus i audio::Voice */
    /** @{ */
    /** @brief Zgłasza odtworzenie dźwięku. */
    audio::Voice Play(audio::Sound& sound);
    /** @brief Zgłasza odtworzenie muzyki (gdy już gra - nic). */
    void Play(audio::Music& music);
    /** @brief Zatrzymuje głosy dźwięku albo muzyki. */
    void Stop(const audio::Source& source);
    /** @brief Zatrzymuje głosy szyny i jej dzieci. */
    void Stop(const audio::Bus& bus);
    /** @brief Liczy głosy dźwięku albo muzyki. */
    int VoiceCount(const audio::Source& source) const;
    /** @brief Liczy głosy szyny i jej dzieci. */
    int VoiceCount(const audio::Bus& bus) const;
    /** @brief Podpina szynę pod rodzica, jeśli nie powstanie cykl. */
    void Reparent(audio::Bus& bus, const std::string& parent);
    /**
     * @brief Ustawia scenę, do której należą głosy zgłaszane od teraz (nullptr - żadna).
     * @details Engine ustawia ją na czas aktualizacji i konfiguracji sceny. Głosy z
     * Source::Global() (domyślnie muzyka) nie należą do sceny - grają dalej po jej zwolnieniu.
     */
    void Owner(const void* scene) { owner_ = scene; }
    /** @brief Zatrzymuje od razu (bez zdarzeń) głosy zwalnianej sceny. */
    void StopOwner(const void* scene);
    /** @brief Zapisuje położenie słuchacza (node::sound::Listener) w scenie. */
    void Listen(const void* listener, const void* scene, Vector2 position);
    /** @brief Usuwa słuchacza (gdy to on jest zapisany). */
    void Unlisten(const void* listener);
    /** @brief Podaje położenie słuchacza w scenie, jeśli scena go ma. */
    std::optional<Vector2> ListenerIn(const void* scene) const;
    /** @brief Poprawia limit głosów szyny (szyna główna: od 1 do rozmiaru puli). */
    int ClampVoices(const audio::Bus& bus, int count) const;
    /** @} */

    /** @brief Głos w puli. */
    struct Slot {
        std::uint32_t generation = 1; /**< Generacja (rośnie przy każdym zwolnieniu). */
        bool used = false;            /**< Czy miejsce jest zajęte. */
        bool started = false;         /**< Czy głos już gra (false - czeka na koniec klatki). */
        bool paused = false;          /**< Czy dźwięk jest wstrzymany w raylib. */
        audio::Source* source = nullptr; /**< Dźwięk albo muzyka, z której powstał głos. */
        audio::Sound* sound = nullptr;   /**< Dźwięk (nullptr dla muzyki). */
        audio::Music* track = nullptr;   /**< Muzyka (nullptr dla dźwięku). */
        std::string bus;              /**< Szyna (ustalona przy Play()). */
        std::string path;             /**< Wylosowany plik. */
        bool stream = false;          /**< Czy głos gra strumień (music), a nie alias (sound). */
        ::Sound alias{};              /**< Alias próbek pliku. */
        ::Music music{};              /**< Strumień (Streamed() albo Looped()). */
        std::shared_ptr<const AudioClip> clip; /**< Dane pliku (poza Streamed()), póki głos gra. */
        float fade = 1.0f;            /**< Bieżący poziom narastania/wyciszania (0-1). */
        float fadeRate = 0.0f;        /**< Zmiana fade na sekundę. */
        bool stopping = false;        /**< Czy głos wycisza się po Stop() (potem zwolnienie). */
        bool userPaused = false;      /**< Czy wstrzymany przez Voice::Pause(). */
        float time = 0.0f;            /**< Czas odtwarzania liczony przez silnik (alias). */
        float seek = -1.0f;           /**< Przewinięcie do wykonania po starcie (< 0 - brak). */
        std::uint64_t order = 0;      /**< Numer Play() - najstarszy głos ma najmniejszy. */
        std::function<void(audio::Voice&)> onFinish; /**< Zdarzenie końca (Voice::OnFinish()). */
        const void* owner = nullptr;  /**< Scena, do której należy głos (nullptr - żadna). */
        float spatialGain = 1.0f;     /**< Tłumienie z odległości (node::sound::Emitter). */
        float spatialPan = 0.0f;      /**< Panorama z położenia (node::sound::Emitter). */
        float volume = 1.0f;          /**< Głośność głosu (Voice::Volume()). */
        float pitch = 1.0f;           /**< Wysokość głosu (Voice::Pitch()). */
        float pan = 0.0f;             /**< Panorama głosu (Voice::Pan()). */
        float variation = 1.0f;       /**< Wylosowana zmiana wysokości (PitchVariation()). */
    };

    /** @brief Podaje głos uchwytu albo nullptr dla martwego uchwytu. */
    Slot* Find(const audio::Voice& voice);
    /** @copydoc Find() */
    const Slot* Find(const audio::Voice& voice) const;

    /** @brief Zatrzymuje głos. */
    void Stop(Slot& slot);
    /** @brief Przewija głos (strumień) albo zapisuje w logu, że się nie da. */
    void Seek(Slot& slot, float seconds);
    /** @brief Podaje miejsce odtwarzania głosu w sekundach. */
    float PlaybackTime(const Slot& slot) const;
    /** @brief Podaje długość pliku głosu w sekundach (0 przed startem). */
    float Duration(const Slot& slot) const;

  private:
    /** @brief Stan szyny razem z rodzicami, liczony raz na klatkę. */
    struct BusState {
        float gain = 1.0f;   /**< Iloczyn głośności (0 przy wyciszeniu). */
        bool paused = false; /**< Czy szyna albo rodzic jest wstrzymany. */
    };

    /** @brief Wycisza pozostałe głosy szyny Exclusive() po starcie głosu slot. */
    void Replace(const Slot& slot);
    /** @brief Zajmuje miejsce w puli dla nowego głosu (wspólna część Play()). */
    audio::Voice Request(audio::Source& source, int maxVoices);
    /** @brief Zapisuje brak urządzenia (raz); false - urządzenie jest. */
    bool WarnNoDevice();
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
    void MakeRoom(const audio::Source& source, int maxVoices);
    /** @brief Wywołuje zdarzenia końca zebrane w tej klatce. */
    void Notify();
    /** @brief Ustawia głośność, wysokość i panoramę głosu w raylib. */
    void Apply(Slot& slot, const BusState& bus);
    /** @brief Liczy stan szyny razem z rodzicami. */
    BusState StateOf(const std::string& bus) const;
    /** @brief Sprawdza, czy szyna jest szyną ancestor albo jej potomkiem. */
    bool IsWithin(const std::string& bus, const std::string& ancestor) const;
    /** @brief Wybiera plik dla nowego głosu. */
    std::string PickFile(audio::Source& source);
    /** @brief Uchwyt miejsca. */
    audio::Voice Handle(std::size_t index) const;

    static inline AudioMixer* current_ = nullptr; /**< Mikser silnika. */

    LogFunction log_;                             /**< Zapis w logu. */
    ::Assets& assets_;                            /**< Dane plików i ścieżki. */
    bool ready_ = false;                          /**< Czy urządzenie jest otwarte. */
    bool warnedNoDevice_ = false;                 /**< Czy zapisano brak urządzenia. */
    std::map<std::string, audio::Sound> sounds_;  /**< Dźwięki według nazw. */
    std::map<std::string, audio::Music> music_;   /**< Muzyka według nazw. */
    std::map<std::string, audio::Bus> buses_;     /**< Szyny według nazw ("" - główna). */
    ::Audio facade_{*this};                       /**< Fasada dla gry. */
    std::vector<Slot> slots_;                     /**< Pula głosów (stały rozmiar). */
    std::uint64_t order_ = 0;                     /**< Licznik Play(). */
    const void* owner_ = nullptr;                 /**< Scena dla nowych głosów. */
    const void* listener_ = nullptr;              /**< Słuchacz (node::sound::Listener). */
    const void* listenerScene_ = nullptr;         /**< Scena słuchacza. */
    Vector2 listenerPosition_{};                  /**< Położenie słuchacza w świecie. */
    bool warnedSeek_ = false;                     /**< Czy zapisano brak przewijania aliasu. */
    /** Zdarzenia końca do wywołania na końcu klatki. */
    std::vector<std::pair<std::function<void(audio::Voice&)>, audio::Voice>> finished_;
    /** Muzyka, która w tej klatce doszła do końca (Music::OnFinish()). */
    std::vector<audio::Music*> finishedMusic_;
};
