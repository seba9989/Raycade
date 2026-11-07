/**
 * @file Typeface.hpp
 * @brief Czcionka TTF/OTF z polskimi znakami: wczytywanie, pomiar i rysowanie tekstu.
 */
#pragma once

#include <raylib.h>

#include <memory>
#include <optional>
#include <string>

/**
 * @brief Czcionka do rysowania tekstu (opakowanie raylib Font).
 * @details Domyślny font raylib (pusty Typeface) ma tylko znaki ASCII - polskie litery
 * wyświetla jako znaki zapytania. Load() wczytuje plik TTF/OTF razem z ASCII i polskimi
 * literami (ąćęłńóśźż, wielkie i małe), a w razie potrzeby z dodatkowymi znakami.
 *
 * Czcionka to zasób GPU (tekstura z glifami) - wczytuje się ją po otwarciu okna, nie kopiuje,
 * a współdzieli przez std::shared_ptr. Kształt shapes::Text i pole ui::TextField przyjmują ją
 * przez Font(); bez tego szukają DefaultTypeface w kontekście (Contextable), a na końcu
 * używają fontu raylib.
 * @code{.cpp}
 * if (auto font = Typeface::Load(engine.AssetPath("Roboto.ttf"), 32))
 *     engine.ProvideContext<DefaultTypeface>(std::make_shared<const Typeface>(std::move(*font)));
 * scene.Spawn<shapes::Text>().Content("Zażółć gęślą jaźń").FontSize(24);
 * @endcode
 */
class Typeface {
  public:
    /** @brief Polskie litery dołączane zawsze przez Load() (oprócz ASCII 32-126). */
    static constexpr const char* kPolishLetters = "ąćęłńóśźżĄĆĘŁŃÓŚŹŻ";

    /** @brief Font domyślny raylib (tylko ASCII, wymaga otwartego okna przy rysowaniu). */
    Typeface() = default;

    /**
     * @brief Wczytuje czcionkę z pliku TTF albo OTF.
     * @details Glify są rasteryzowane raz, w rozmiarze @p size - tekst dużo większy od niego
     * będzie rozmyty, dlatego warto wczytać czcionkę w największym używanym rozmiarze.
     * @param path Ścieżka pliku.
     * @param size Rozmiar rasteryzacji glifów w pikselach (domyślnie 32).
     * @param extraCharacters Dodatkowe znaki UTF-8 (np. "€°ß"); ASCII i polskie litery są
     * zawsze.
     * @return Czcionka albo brak wartości, gdy pliku nie da się wczytać.
     */
    static std::optional<Typeface> Load(const std::string& path, int size = 32,
                                        const std::string& extraCharacters = "");

    /**
     * @brief Udostępnia wspólny font domyślny (do użycia, gdy nie wybrano innej czcionki).
     * @return Pusty Typeface, czyli font raylib.
     */
    static const Typeface& Default();

    /** @brief Zwalnia czcionkę z pamięci i karty graficznej. */
    ~Typeface();

    /** @brief Czcionka to zasób GPU - nie można jej kopiować (współdzieli się ją przez shared_ptr).
     */
    Typeface(const Typeface&) = delete;
    /** @brief Czcionka to zasób GPU - nie można jej kopiować. */
    Typeface& operator=(const Typeface&) = delete;
    /** @brief Przenosi czcionkę; źródło staje się fontem domyślnym. */
    Typeface(Typeface&& other) noexcept;
    /** @brief Przenosi czcionkę; poprzednia jest zwalniana. */
    Typeface& operator=(Typeface&& other) noexcept;

    /** @brief Sprawdza, czy to font domyślny raylib (bez polskich znaków). */
    bool IsDefault() const { return !loaded_; }

    /**
     * @brief Podaje domyślny odstęp między znakami dla rozmiaru tekstu.
     * @details Tak samo jak DrawText() z raylib: rozmiar / 10, co najmniej 1 piksel.
     * @param size Rozmiar tekstu w pikselach.
     * @return Odstęp w pikselach.
     */
    static float DefaultSpacing(float size);

    /**
     * @brief Mierzy tekst (także wielowierszowy, z '\n').
     * @param text Tekst UTF-8.
     * @param size Wysokość tekstu w pikselach.
     * @param spacing Odstęp między znakami w pikselach.
     * @return Szerokość i wysokość w pikselach.
     */
    Vector2 Measure(const std::string& text, float size, float spacing) const;

    /**
     * @brief Rysuje tekst w bieżącym układzie (z macierzą obiektu i kamerą).
     * @param text Tekst UTF-8.
     * @param position Lewy górny róg.
     * @param size Wysokość tekstu w pikselach.
     * @param spacing Odstęp między znakami w pikselach.
     * @param color Kolor.
     */
    void Draw(const std::string& text, Vector2 position, float size, float spacing,
              Color color) const;

    /** @brief Udostępnia font raylib (font domyślny dla pustego Typeface). */
    const ::Font& Raw() const;

  private:
    void Release();

    ::Font font_{};       /**< Wczytany font (puste dla fontu domyślnego). */
    bool loaded_ = false; /**< Czy font_ został wczytany z pliku. */
};

/**
 * @brief Czcionka domyślna dla tekstów w poddrzewie (wartość kontekstu).
 * @details shapes::Text i ui::TextField bez własnej czcionki (Font()) szukają tej wartości
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
inline const Typeface& ResolveTypeface(const std::shared_ptr<const Typeface>& own,
                                       const DefaultTypeface* context) {
    if (own) return *own;
    if (context && context->typeface) return *context->typeface;
    return Typeface::Default();
}
