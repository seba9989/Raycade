/**
 * @file Typeface.hpp
 * @brief Czcionka TTF/OTF: atlasy glifów dla każdego rozmiaru, układ, pomiar i rysowanie tekstu.
 */
#pragma once

#include <raylib.h>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <raycade/render/Shared.hpp> // IWYU pragma: export

/** @brief Wyrównanie wierszy tekstu w obrębie bloku. */
enum class TextAlign {
    Left,   /**< Do lewej. */
    Center, /**< Do środka. */
    Right,  /**< Do prawej. */
};

/**
 * @brief Parametry układu tekstu: rozmiar, odstępy, łamanie i wyrównanie.
 * @details Pierwsze dwa pola można podać skrótowo: `TextStyle{24, 0}`.
 */
struct TextStyle {
    float size = 20.0f;                /**< Wysokość tekstu w pikselach. */
    float spacing = 0.0f;              /**< Dodatkowy odstęp między znakami w pikselach. */
    float lineHeight = 1.2f;           /**< Odległość wierszy jako wielokrotność size. */
    float wrapWidth = 0.0f;            /**< Szerokość łamania wierszy; 0 - bez łamania. */
    TextAlign align = TextAlign::Left; /**< Wyrównanie wierszy. */
};

/** @brief Ustawienia wczytywania czcionki (Typeface::Load()). */
struct TypefaceOptions {
    /**
     * @brief Znaki UTF-8 przygotowane od razu oprócz ASCII i polskich liter (np. "€°ß").
     * @details Pozostałe znaki są dokładane same przy pierwszym użyciu - lista tylko oszczędza
     * przebudowy atlasu w trakcie gry.
     */
    std::string characters;
    /** @brief true - filtr dwuliniowy (gładkie krawędzie), false - punktowy (font pikselowy). */
    bool smooth = true;
};

/**
 * @brief Czcionka do rysowania tekstu.
 * @details Load() wczytuje plik TTF/OTF do pamięci (bez okna). Glify są rasteryzowane
 * dopiero przy użyciu, osobno dla każdego rozmiaru na ekranie: tekst 16 px rysuje atlas
 * 16 px, a ten sam tekst powiększony Scale() albo kamerą - atlas w rozmiarze po
 * powiększeniu. Dzięki temu tekst jest ostry w każdej skali, a nie rozmyty przez zmniejszanie
 * lub powiększanie jednego atlasu. Przy rysowaniu bez obrotu glify są wyrównywane do pikseli
 * ekranu. Pamięć trzyma najwyżej kilkanaście ostatnio używanych atlasów.
 *
 * Od razu przygotowane są znaki ASCII i polskie litery (ąćęłńóśźż, wielkie i małe). Inny znak
 * jest dokładany przy pierwszym użyciu (atlasy budują się wtedy od nowa), więc "€" czy "ß"
 * działają bez dodatkowej konfiguracji.
 *
 * Domyślny font raylib (pusty Typeface) ma tylko znaki ASCII - polskie litery wyświetla jako
 * znaki zapytania.
 *
 * Kształt visual::Text i pole ui::TextField przyjmują czcionkę przez Font(); bez tego szukają
 * DefaultTypeface w kontekście (Contextable), a na końcu używają fontu raylib.
 * @code{.cpp}
 * engine.ProvideContext<DefaultTypeface>(engine.Assets().Font("Roboto.ttf"));
 * scene.Spawn<visual::Text>().Content("Zażółć gęślą jaźń").FontSize(24);
 * @endcode
 */
class Typeface {
  public:
    /** @brief Polskie litery przygotowane zawsze przez Load() (oprócz ASCII 32-126). */
    static constexpr const char* kPolishLetters = "ąćęłńóśźżĄĆĘŁŃÓŚŹŻ";

    /** @brief Font domyślny raylib (tylko ASCII, wymaga otwartego okna). */
    Typeface() = default;

    /**
     * @brief Wczytuje czcionkę z pliku TTF albo OTF.
     * @details Nie wymaga otwartego okna - tekstury atlasów powstają przy pierwszym rysowaniu.
     * @code{.cpp}
     * auto pixel = Shared(Typeface::Load(engine.AssetPath("pixel.ttf"), {.smooth = false}));
     * @endcode
     * @param path Ścieżka pliku.
     * @param options Dodatkowe znaki i filtr tekstury.
     * @return Czcionka albo brak wartości, gdy pliku nie da się wczytać.
     */
    static std::optional<Typeface> Load(const std::string& path, TypefaceOptions options = {});

    /**
     * @brief Udostępnia wspólny font domyślny (do użycia, gdy nie wybrano innej czcionki).
     * @return Pusty Typeface, czyli font raylib.
     */
    static const Typeface& Default();

    /** @brief Zwalnia atlasy z pamięci i karty graficznej. */
    ~Typeface();

    /** @brief Czcionka to zasób GPU - nie kopiuje się jej, tylko współdzieli przez shared_ptr. */
    Typeface(const Typeface&) = delete;
    /** @brief Czcionka to zasób GPU - nie można jej kopiować. */
    Typeface& operator=(const Typeface&) = delete;
    /** @brief Przenosi czcionkę; źródło staje się fontem domyślnym. */
    Typeface(Typeface&& other) noexcept;
    /** @brief Przenosi czcionkę; poprzednia jest zwalniana. */
    Typeface& operator=(Typeface&& other) noexcept;

    /** @brief Sprawdza, czy to font domyślny raylib (bez polskich znaków). */
    bool IsDefault() const { return data_.empty(); }

    /** @brief Sprawdza, czy atlasy używają filtru dwuliniowego. */
    bool IsSmooth() const { return smooth_; }

    /**
     * @brief Podaje domyślny odstęp między znakami dla rozmiaru tekstu.
     * @details Font z pliku TTF/OTF ma własne odstępy (advance) zapisane w glifach, więc
     * dodatkowy odstęp wynosi 0. Font domyślny raylib ich nie ma - dla niego, tak jak w
     * DrawText(), odstęp to rozmiar / 10, co najmniej 1 piksel.
     * @param size Rozmiar tekstu w pikselach.
     * @return Odstęp w pikselach.
     */
    float DefaultSpacing(float size) const;

    /**
     * @brief Mierzy tekst ułożony według stylu (wiersze z '\n' i łamanie do wrapWidth).
     * @details Szerokość to najszerszy wiersz, a wysokość: size dla jednego wiersza i size *
     * lineHeight więcej na każdy kolejny.
     * @param text Tekst UTF-8.
     * @param style Parametry układu.
     * @return Szerokość i wysokość w pikselach.
     */
    Vector2 Measure(std::string_view text, const TextStyle& style) const;

    /**
     * @brief Mierzy tekst (skrót bez łamania, z domyślną odległością wierszy).
     * @param text Tekst UTF-8.
     * @param size Wysokość tekstu w pikselach.
     * @param spacing Odstęp między znakami w pikselach.
     * @return Szerokość i wysokość w pikselach.
     */
    Vector2 Measure(std::string_view text, float size, float spacing) const {
        return Measure(text, TextStyle{size, spacing});
    }

    /**
     * @brief Rysuje tekst w bieżącym układzie (z macierzą obiektu i kamerą).
     * @details Rozmiar atlasu wynika z size i skali bieżącego przekształcenia, więc tekst
     * powiększony przez Scale() obiektu albo zoom kamery pozostaje ostry.
     * @param text Tekst UTF-8.
     * @param position Lewy górny róg bloku tekstu.
     * @param style Parametry układu.
     * @param color Kolor.
     */
    void Draw(std::string_view text, Vector2 position, const TextStyle& style, Color color) const;

    /**
     * @brief Rysuje tekst (skrót bez łamania, z domyślną odległością wierszy).
     * @param text Tekst UTF-8.
     * @param position Lewy górny róg.
     * @param size Wysokość tekstu w pikselach.
     * @param spacing Odstęp między znakami w pikselach.
     * @param color Kolor.
     */
    void Draw(
        std::string_view text,
        Vector2 position,
        float size,
        float spacing,
        Color color
    ) const {
        Draw(text, position, TextStyle{size, spacing}, color);
    }

    /**
     * @brief Udostępnia font raylib z atlasem dla podanego rozmiaru (np. do DrawTextEx()).
     * @param pixelSize Rozmiar glifów w pikselach ekranu.
     * @return Font raylib; dla pustego Typeface - font domyślny raylib.
     */
    const ::Font& Raw(float pixelSize = 32.0f) const;

    /** @brief Podaje liczbę atlasów w pamięci (diagnostyka). */
    int AtlasCount() const { return static_cast<int>(atlases_.size()); }

  private:
    /** @brief Atlas glifów jednego rozmiaru. */
    struct Atlas {
        ::Font font{}; /**< Glify, prostokąty i tekstura (po wysłaniu na GPU). */
        std::unordered_map<int, int> glyphs; /**< Znak -> indeks glifu w font.glyphs. */
        Image pending{};                     /**< Obraz atlasu czekający na otwarte okno. */
        std::uint64_t used = 0; /**< Ostatnie użycie (do usuwania najdawniej używanych). */
    };

    /** @brief Atlas dla rozmiaru na ekranie (tworzy go w razie potrzeby). */
    const Atlas& AtlasFor(float pixelSize, bool needTexture) const;

    /** @brief Buduje atlas glifów (na CPU; tekstura - gdy okno jest otwarte). */
    Atlas Build(int pixelSize) const;

    /** @brief Wysyła oczekujący obraz atlasu na kartę graficzną. */
    void Upload(Atlas& atlas) const;

    /** @brief Zwalnia atlas. */
    static void Release(Atlas& atlas);

    /** @brief Zwalnia wszystkie atlasy. */
    void ReleaseAll() const;

    /** @brief Dokłada do zestawu znaki tekstu, których jeszcze nie ma (przebudowa atlasów). */
    void EnsureGlyphs(const std::vector<int>& codepoints) const;

    /** @brief Podaje indeks glifu w atlasie albo -1. */
    int GlyphIndex(const Atlas& atlas, int codepoint) const;

    std::vector<unsigned char> data_;       /**< Plik czcionki; pusty - font raylib. */
    std::string extension_;                 /**< Rozszerzenie (".ttf" / ".otf"). */
    bool smooth_ = true;                    /**< Filtr dwuliniowy. */
    mutable std::vector<int> codepoints_;   /**< Przygotowane znaki. */
    mutable std::unordered_set<int> known_; /**< Znaki z codepoints_ (szybkie sprawdzanie). */
    mutable std::map<int, Atlas> atlases_;  /**< Atlasy według rozmiaru w pikselach. */
    mutable std::uint64_t clock_ = 0;       /**< Licznik użyć atlasów. */
};

/**
 * @brief Czcionka domyślna dla tekstów w poddrzewie (wartość kontekstu).
 * @details visual::Text i ui::TextField bez własnej czcionki (Font()) szukają tej wartości
 * w kontekście: w obiekcie, jego przodkach, scenie i silniku.
 * @code{.cpp}
 * engine.ProvideContext<DefaultTypeface>(font);        // cała gra
 * menu.ProvideContext<DefaultTypeface>(titleFont);     // tylko poddrzewo menu
 * @endcode
 */
struct DefaultTypeface {
    std::shared_ptr<const Typeface> typeface; /**< Czcionka; nullptr - font raylib. */
};

/**
 * @brief Wybiera czcionkę tekstu: własną obiektu, z kontekstu albo domyślną raylib.
 * @param own Czcionka ustawiona na obiekcie (Font()) albo nullptr.
 * @param context Wartość z kontekstu (`Context<DefaultTypeface>()`) albo nullptr.
 * @return Czcionka do pomiaru i rysowania.
 */
inline const Typeface& ResolveTypeface(
    const std::shared_ptr<const Typeface>& own,
    const DefaultTypeface* context
) {
    if (own) return *own;
    if (context && context->typeface) return *context->typeface;
    return Typeface::Default();
}
