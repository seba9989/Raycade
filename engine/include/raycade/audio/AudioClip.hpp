/**
 * @file AudioClip.hpp
 * @brief Klasa AudioClip - dane jednego pliku dźwiękowego, współdzielone przez głosy.
 */
#pragma once

#include <raylib.h>

#include <optional>
#include <span>
#include <string>
#include <vector>

#include <raycade/render/Shared.hpp> // IWYU pragma: export

/**
 * @brief Dane jednego pliku dźwiękowego (WAV, OGG, MP3, FLAC...), współdzielone przez głosy.
 * @details Load() nie wymaga urządzenia audio: czyta plik i dekoduje go, żeby sprawdzić, czy
 * jest poprawny. Zdekodowane próbki czekają w pamięci RAM i trafiają do urządzenia przy
 * pierwszym odtworzeniu (Sound()) - tak jak Bitmap wysyła teksturę na kartę graficzną dopiero
 * przy rysowaniu. Plik zostaje też w pamięci w postaci zakodowanej - z niego grają zapętlone
 * strumienie (Source::Looped() bez Sound::Streamed()).
 *
 * Gra zwykle nie używa klipów wprost - pliki wczytuje mikser (Engine::Audio()) przy pierwszym
 * Play(). Assets::Clip() pozwala sprawdzić plik albo wczytać go zawczasu: klip trzymany przez grę
 * nie jest zwalniany przy zmianie scen, więc pierwsze Play() nie czeka na dysk.
 * @code{.cpp}
 * // w konfiguracji sceny: boss.wav zostaje w pamięci, dopóki lambda trzyma wskaźnik
 * scene.OnUpdate([roar = engine.Assets().Clip("roar.wav")](float) {
 *     if (bossAppears) Engine::Get().Audio().Sound("roar.wav").Play(); // bez czekania na dysk
 * });
 * @endcode
 */
class AudioClip {
  public:
    /** @brief Pusty klip (IsLoaded() zwraca false); głos z nim gra ciszą. */
    AudioClip() = default;

    /**
     * @brief Wczytuje plik dźwiękowy.
     * @details Obsługiwane formaty zależą od kompilacji raylib (domyślnie WAV, OGG, MP3, QOA,
     * FLAC). Nie wymaga urządzenia audio.
     * @param path Ścieżka pliku.
     * @return Klip albo brak wartości, gdy pliku nie da się wczytać ani zdekodować.
     */
    static std::optional<AudioClip> Load(const std::string& path);

    /** @brief Zwalnia próbki (z urządzenia - tylko gdy jest jeszcze otwarte). */
    ~AudioClip();

    /** @brief Klip to zasób urządzenia - nie kopiuje się go, tylko współdzieli przez shared_ptr. */
    AudioClip(const AudioClip&) = delete;
    /** @brief Klip to zasób urządzenia - nie można go kopiować. */
    AudioClip& operator=(const AudioClip&) = delete;
    /** @brief Przenosi klip; źródło zostaje puste. */
    AudioClip(AudioClip&& other) noexcept;
    /** @brief Przenosi klip; poprzedni jest zwalniany. */
    AudioClip& operator=(AudioClip&& other) noexcept;

    /** @brief Sprawdza, czy klip ma dane z pliku (pusty - zastępczy, gra ciszą). */
    bool IsLoaded() const { return !bytes_.empty(); }

    /** @brief Podaje długość dźwięku w sekundach (znaną od razu po Load()). */
    float Duration() const { return duration_; }

    /**
     * @brief Podaje próbki w urządzeniu audio, wysyłając je tam przy pierwszym wywołaniu.
     * @details Po wysłaniu próbki znikają z pamięci RAM klipu. Źródło aliasów
     * (LoadSoundAlias()), którymi gra każdy głos.
     * @return Dźwięk raylib.
     * @pre Urządzenie audio jest otwarte, a klip wczytany (IsLoaded()).
     */
    const ::Sound& Sound() const;

    /** @brief Podaje zakodowany plik - źródło strumienia z pamięci (LoadMusicStreamFromMemory). */
    std::span<const unsigned char> Bytes() const { return bytes_; }

    /** @brief Podaje rozszerzenie pliku (np. ".ogg") - typ dla LoadMusicStreamFromMemory(). */
    const std::string& Type() const { return type_; }

  private:
    /** @brief Zwalnia próbki i dane pliku. */
    void Release();

    std::vector<unsigned char> bytes_; /**< Zakodowany plik; pusty - klip pusty. */
    std::string type_;                 /**< Rozszerzenie pliku. */
    float duration_ = 0.0f;            /**< Długość w sekundach. */
    mutable ::Wave pending_{};         /**< Próbki czekające na urządzenie audio. */
    mutable ::Sound sound_{};          /**< Próbki w urządzeniu (po pierwszym Sound()). */
};
