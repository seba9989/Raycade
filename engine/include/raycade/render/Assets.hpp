/**
 * @file Assets.hpp
 * @brief Klasa Assets - pamięć podręczna zasobów z plików (bitmapy, czcionki, arkusze).
 */
#pragma once

#include <cstddef>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include <raycade/render/Bitmap.hpp>
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
 * Plik, którego nie da się wczytać, nie daje nullptr: bitmapa zostaje zastąpiona szachownicą
 * magenta-czarną (Missing()), arkusz - jedną klatką z tą szachownicą, a czcionka - fontem
 * domyślnym raylib. Czy plik się wczytał, mówi Bitmap::IsLoaded(), SpriteSheet::IsLoaded()
 * i Typeface::IsDefault(). Błąd jest widoczny na
 * ekranie, a w logu pojawia się jeden wpis na plik, nawet jeśli gra prosi o niego co klatkę.
 *
 * Silnik zwalnia nieużywane zasoby (Purge()) po zwolnieniu albo ponownym wczytaniu sceny,
 * a wszystkie (Clear()) - przy zamykaniu, przed zamknięciem okna.
 * @code{.cpp}
 * Assets& assets = Engine::Get().Assets();
 * auto coin = assets.Bitmap("coin.png");
 * auto font = assets.Font("Roboto.ttf");
 * auto hero = assets.Sheet("hero.json"); // Aseprite / TexturePacker
 * for (int i = 0; i < 10; ++i)
 *     scene.Spawn<shapes::Sprite>().Sheet(coin, {16, 16}).Play().Position({i * 40.0f, 0});
 * @endcode
 *
 * Silnik udostępnia swoje Assets w kontekście (Contextable), dlatego shapes::Sprite::Sheet()
 * z nazwą pliku korzysta z tej samej pamięci podręcznej. Poddrzewo może podać własne:
 * `menu.SetContext(menuAssets)`.
 */
class Assets {
  public:
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
     * @brief Podaje czcionkę z pliku TTF/OTF, wczytując ją przy pierwszym użyciu.
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
     * @brief Podaje arkusz klatek z pliku JSON (Aseprite, TexturePacker), wczytując go przy
     * pierwszym użyciu.
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

    /** @brief Podaje liczbę zasobów w pamięci podręcznej. */
    std::size_t Count() const { return bitmaps_.size() + fonts_.size() + sheets_.size(); }

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
    /** @brief Zapisuje błąd wczytania w logu (raz na klucz). */
    bool Failed(const std::string& key, const std::string& path);

    std::string directory_; /**< Katalog nazw względnych. */
    std::unordered_map<std::string, std::shared_ptr<const ::Bitmap>> bitmaps_;   /**< Bitmapy. */
    std::unordered_map<std::string, std::shared_ptr<const Typeface>> fonts_;     /**< Czcionki. */
    std::unordered_map<std::string, std::shared_ptr<const SpriteSheet>> sheets_; /**< Arkusze. */
    std::shared_ptr<const SpriteSheet> missingSheet_; /**< Arkusz zastępczy. */
    std::set<std::string> failed_;                    /**< Klucze, których nie wczytano. */
    std::shared_ptr<const ::Bitmap> missing_;         /**< Bitmapa zastępcza. */
    std::shared_ptr<const Typeface> defaultTypeface_; /**< Font raylib dla błędów. */
};
