/**
 * @file Assets.hpp
 * @brief Klasa Assets - pamięć podręczna zasobów z plików (bitmapy, czcionki, arkusze, shadery,
 * dźwięki).
 */
#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>

#include <raycade/audio/AudioClip.hpp>
#include <raycade/engine/EngineTypes.hpp>
#include <raycade/render/Bitmap.hpp>
#include <raycade/render/ShaderProgram.hpp>
#include <raycade/render/SpriteSheet.hpp>
#include <raycade/render/Typeface.hpp>

/**
 * @brief Wczytuje zasoby z plików raz i udostępnia je wszystkim obiektom.
 * @details Ten sam plik pobrany drugi raz to ten sam obiekt w pamięci (i ta sama tekstura na
 * karcie graficznej), więc dziesięć sprite'ów z "coin.png" nie wczytuje pliku dziesięć razy.
 * Zasoby są zwracane jako `std::shared_ptr<const T>`, czyli tak, jak przyjmują je obiekty sceny.
 *
 * Nazwy względne są liczone od katalogu Directory() - silnik ustawia tam katalog assets/ obok
 * pliku wykonywalnego (Engine::AssetPath()). Ścieżka bezwzględna jest używana bez zmian.
 *
 * Plik, którego nie da się wczytać, nie daje nullptr, tylko zasób zastępczy, który da się
 * użyć jak prawdziwy:
 *
 * | Zasób         | Metoda   | Zastępstwo                 | Wymaga                 |
 * |---------------|----------|----------------------------|------------------------|
 * | Bitmap        | Bitmap() | szachownica magenta-czarna | -                      |
 * | SpriteSheet   | Sheet()  | jedna klatka z szachownicą | -                      |
 * | Typeface      | Font()   | font domyślny raylib       | -                      |
 * | ShaderProgram | Shader() | pusty shader (bez efektu)  | otwartego okna         |
 * | AudioClip     | Clip()   | pusty klip (cisza)         | - (granie: urządzenia) |
 *
 * Czy plik się wczytał, mówi IsLoaded() każdego z tych typów. Błąd jest widoczny (albo
 * słyszalny), a w logu pojawia się jeden wpis na plik, nawet jeśli gra prosi o niego co klatkę.
 *
 * Silnik zwalnia nieużywane zasoby (Purge()) po zwolnieniu albo ponownym wczytaniu sceny,
 * a wszystkie (Clear()) - przy zamykaniu, przed zamknięciem okna.
 * @code{.cpp}
 * Assets& assets = Engine::Get().Assets();
 * auto coin = assets.Bitmap("coin.png");
 * auto font = assets.Font("Roboto.ttf");
 * auto hero = assets.Sheet("hero.json"); // Aseprite / TexturePacker
 * auto wave = assets.Shader("wave.fs");  // tylko shader fragmentów
 * for (int i = 0; i < 10; ++i)
 *     scene.Spawn<visual::Sprite>().Sheet(coin, {16, 16}).Play().Position({i * 40.0f, 0});
 * @endcode
 *
 * Silnik udostępnia swoje Assets w kontekście (Contextable), dlatego visual::Sprite::Sheet()
 * z nazwą pliku korzysta z tej samej pamięci podręcznej (tak samo visual::Text::Font()
 * i Drawable::Shader()). Poddrzewo może podać własne: `menu.SetContext(menuAssets)`.
 * Bez Assets w kontekście te metody używają Local().
 */
class Assets {
  public:
    /** @brief Zapis w logu (poziom i treść). */
    using LogFunction = std::function<void(LogLevel, const std::string&)>;

    /**
     * @brief Podaje pamięć podręczną bez katalogu (nazwy względem katalogu roboczego).
     * @details Używają jej settery z nazwą pliku (np. visual::Sprite::Sheet()), gdy w kontekście
     * nie ma Assets - dzięki temu brakujący plik daje ten sam zasób zastępczy i wpis w logu.
     * Nikt jej nie czyści - zasoby żyją do końca programu.
     */
    static Assets& Local();

    /**
     * @brief Ustawia, dokąd trafiają błędy wczytania.
     * @details Silnik kieruje je do Engine::Log(). Bez tego - do logu raylib (TraceLog).
     * @param log Funkcja zapisu albo pusta - log raylib.
     * @return Ten sam obiekt.
     */
    Assets& Logger(LogFunction log);

    /**
     * @brief Ustawia katalog, od którego liczone są nazwy względne.
     * @param directory Katalog (z ukośnikiem na końcu albo bez); pusty - katalog roboczy.
     * @return Ten sam obiekt.
     */
    Assets& Directory(std::string directory);

    /** @brief Podaje katalog zasobów. */
    const std::string& Directory() const { return directory_; }

    /**
     * @brief Zamienia nazwę zasobu na ścieżkę pliku.
     * @param name Nazwa względna (np. "postac/hero.png") albo ścieżka bezwzględna.
     * @return Ścieżka pliku (znormalizowana - bez "." i ".."; służy też jako klucz pamięci).
     */
    std::string Path(const std::string& name) const;

    /**
     * @brief Podaje bitmapę z pliku, wczytując ją przy pierwszym użyciu.
     * @param name Nazwa pliku (patrz Path()).
     * @return Bitmapa albo Missing(), gdy pliku nie da się wczytać.
     */
    std::shared_ptr<const ::Bitmap> Bitmap(const std::string& name);

    /**
     * @brief Podaje czcionkę (Typeface) z pliku TTF/OTF, wczytując ją przy pierwszym użyciu.
     * @details Ten sam plik z innymi ustawieniami to osobna czcionka. Rozmiar nie jest
     * parametrem - Typeface tworzy atlasy dla rozmiarów, w których tekst jest rysowany.
     * @code{.cpp}
     * auto pixel = assets.Font("pixel.ttf", {.smooth = false});
     * @endcode
     * @param name Nazwa pliku (patrz Path()).
     * @param options Dodatkowe znaki i filtr (patrz Typeface::Load()).
     * @return Czcionka albo font domyślny raylib, gdy pliku nie da się wczytać.
     */
    std::shared_ptr<const Typeface> Font(
        const std::string& name,
        const TypefaceOptions& options = {}
    );

    /**
     * @brief Podaje zastępczą bitmapę (szachownica 16 x 16, magenta i czarny).
     * @details Bitmapa ma znacznik Bitmap::Placeholder(), więc jej IsLoaded() zwraca false.
     * @return Wspólna bitmapa zastępcza.
     */
    std::shared_ptr<const ::Bitmap> Missing();

    /**
     * @brief Podaje arkusz klatek (SpriteSheet) z pliku JSON (Aseprite, TexturePacker),
     * wczytując go przy pierwszym użyciu.
     * @details Obraz z meta.image (względem pliku JSON) też trafia do pamięci podręcznej -
     * Bitmap() z tym samym plikiem zwraca ten sam obiekt. Arkusz jest tylko do odczytu; żeby
     * dopisać animacje albo zdarzenia klatek, robi się kopię:
     * @code{.cpp}
     * auto hero = std::make_shared<SpriteSheet>(*assets.Sheet("hero.json"));
     * hero->Event("run", 2, "krok");
     * @endcode
     * Format i obsługiwane pola opisuje SpriteSheet::Load().
     * @param name Nazwa pliku (patrz Path()).
     * @return Arkusz albo arkusz z jedną klatką szachownicy, gdy pliku albo obrazu nie da się
     * wczytać.
     */
    std::shared_ptr<const SpriteSheet> Sheet(const std::string& name);

    /**
     * @brief Podaje shader z pliku GLSL (tylko fragmentów), wczytując go przy pierwszym użyciu.
     * @details Shader jest wspólny dla wszystkich, którzy pobiorą ten plik - razem z wartościami
     * uniformów. Do efektów, które każdy obiekt ustawia inaczej, wczytuje się osobny program:
     * `Shared(ShaderProgram::Load(...))`.
     * @code{.cpp}
     * auto wave = assets.Shader("wave.fs");
     * wave->Uniform("time", static_cast<float>(engine.Time())); // dla wszystkich z wave.fs
     * @endcode
     * @param fragment Plik shadera fragmentów (patrz Path()).
     * @return Shader albo pusty shader (IsLoaded() == false, obiekt rysuje się bez efektu),
     * gdy pliku nie da się wczytać lub skompilować albo okno nie jest jeszcze otwarte.
     * @pre Okno jest otwarte (kontekst OpenGL) - np. w konfiguracji sceny.
     */
    std::shared_ptr<ShaderProgram> Shader(const std::string& fragment);

    /**
     * @brief Podaje shader z dwóch plików GLSL, wczytując go przy pierwszym użyciu.
     * @param vertex Plik shadera wierzchołków albo pusty napis (domyślny shader raylib).
     * @param fragment Plik shadera fragmentów albo pusty napis (domyślny shader raylib).
     * @return Shader albo pusty shader (patrz Shader(const std::string&)).
     */
    std::shared_ptr<ShaderProgram> Shader(const std::string& vertex, const std::string& fragment);

    /**
     * @brief Podaje dane pliku dźwiękowego, wczytując je przy pierwszym użyciu.
     * @details Z tej metody korzysta mikser (Engine::Audio()) przy każdym Play(). Gra może wczytać plik
     * zawczasu i trzymać wynik, żeby pierwsze odtworzenie nie czekało na dysk (patrz AudioClip).
     * @param name Nazwa pliku (patrz Path()).
     * @return Klip albo pusty klip (IsLoaded() == false, gra ciszą), gdy pliku nie da się
     * wczytać.
     */
    std::shared_ptr<const AudioClip> Clip(const std::string& name);

    /**
     * @brief Zapisuje w logu błąd wczytania zasobu - raz na klucz, do następnego Purge().
     * @details Dla zasobów wczytywanych poza Assets, które mają się zachowywać tak samo (np.
     * dźwięk Sound::Streamed() albo muzyka czytane z dysku przez każdy głos).
     * @param key Klucz błędu (np. ścieżka z dopiskiem rodzaju).
     * @param kind Rodzaj zasobu w dopełniaczu, np. "bitmapy".
     * @param path Ścieżka pliku.
     * @param fallback Co zamiast zasobu, np. "cisza".
     */
    void Report(
        const std::string& key,
        std::string_view kind,
        const std::string& path,
        std::string_view fallback
    );

    /** @brief Sprawdza, czy klucz ma zapisany błąd (Report()) od ostatniego Purge(). */
    bool HasFailed(const std::string& key) const { return failed_.contains(key); }

    /** @brief Podaje liczbę zasobów w pamięci podręcznej. */
    std::size_t Count() const {
        return bitmaps_.size() + fonts_.size() + sheets_.size() + shaders_.size() + sounds_.size();
    }

    /**
     * @brief Zwalnia zasoby, których nie używa już nikt poza pamięcią podręczną.
     * @details Zapomina też pliki, których nie udało się wczytać - następna prośba spróbuje
     * ponownie (np. po skopiowaniu brakującego pliku).
     * @return Liczba zwolnionych zasobów.
     */
    std::size_t Purge();

    /**
     * @brief Zwalnia wszystkie zasoby z pamięci podręcznej.
     * @details Obiekty, które nadal trzymają zasób, zachowują go do swojego końca.
     * @return Ten sam obiekt.
     */
    Assets& Clear();

  private:
    LogFunction log_;       /**< Zapis błędów; pusty - log raylib. */
    std::string directory_; /**< Katalog nazw względnych. */
    std::unordered_map<std::string, std::shared_ptr<const ::Bitmap>> bitmaps_;   /**< Bitmapy. */
    std::unordered_map<std::string, std::shared_ptr<const Typeface>> fonts_;     /**< Czcionki. */
    std::unordered_map<std::string, std::shared_ptr<const SpriteSheet>> sheets_; /**< Arkusze. */
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> shaders_;    /**< Shadery. */
    std::unordered_map<std::string, std::shared_ptr<const AudioClip>> sounds_;   /**< Dźwięki. */
    std::shared_ptr<const SpriteSheet> missingSheet_; /**< Arkusz zastępczy. */
    std::set<std::string> failed_;                    /**< Klucze, których nie wczytano. */
    std::shared_ptr<const ::Bitmap> missing_;         /**< Bitmapa zastępcza. */
    std::shared_ptr<const Typeface> defaultTypeface_; /**< Font raylib dla błędów. */
    std::shared_ptr<ShaderProgram> missingShader_;    /**< Pusty shader dla błędów. */
    std::shared_ptr<const AudioClip> silentClip_;     /**< Pusty klip dla błędów. */
};
