/**
 * @file Text.hpp
 * @brief Kształt Text - napis z wybraną czcionką, wyrównaniem i łamaniem wierszy.
 */
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <raycade/entity/Entity.hpp>
#include <raycade/render/Typeface.hpp>

namespace shapes {

/**
 * @brief Napis (także wielowierszowy), którego pozycja oznacza punkt zaczepienia.
 * @details W odróżnieniu od ::DrawText() w OnDraw() napis jest obiektem sceny: przesuwa się
 * z rodzicem i kamerą, obraca, skaluje, ma warstwę i reaguje na mysz w swoim prostokącie.
 * Powiększony przez Scale() albo kamerę pozostaje ostry (Typeface rasteryzuje glify
 * w rozmiarze na ekranie).
 *
 * Kolor ustawia Paint(). Czcionkę wybiera Font(); bez niej napis używa DefaultTypeface
 * z kontekstu, a na końcu fontu raylib (bez polskich znaków - patrz Typeface).
 *
 * Wiersze zaczyna '\n', a WrapWidth() łamie dłuższe wiersze na spacjach. Align() wyrównuje
 * wiersze w bloku, a LineHeight() ustala ich odległość.
 *
 * Punkt zaczepienia leży w pozycji obiektu i wokół niego działa obrót i skala. To Origin()
 * w pikselach plus Anchor() jako część rozmiaru napisu - domyślnie lewy górny róg.
 * @code{.cpp}
 * scene.Spawn<shapes::Text>()
 *     .Content("Koniec gry\nNaciśnij Enter")
 *     .FontSize(32)
 *     .Align(TextAlign::Center)
 *     .Anchor({0.5f, 0.5f})          // środek napisu w pozycji
 *     .Position({400, 300})
 *     .ScreenSpace();
 * @endcode
 */
class Text : public Entity {
  public:
    /**
     * @brief Ustawia treść napisu.
     * @param content Tekst UTF-8; '\n' zaczyna nowy wiersz.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Content(this Self& self, std::string content) {
        self.content_ = std::move(content);
        self.size_.reset();
        return self;
    }

    /** @brief Podaje treść napisu. */
    const std::string& Content() const { return content_; }

    /**
     * @brief Ustawia wysokość tekstu w pikselach.
     * @param size Wysokość (domyślnie 20).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& FontSize(this Self& self, float size) {
        self.fontSize_ = size;
        self.size_.reset();
        return self;
    }

    /** @brief Podaje wysokość tekstu. */
    float FontSize() const { return fontSize_; }

    /**
     * @brief Ustawia odstęp między znakami.
     * @param spacing Odstęp w pikselach; ujemna wartość - domyślny
     * (Typeface::DefaultSpacing(): 0 dla TTF/OTF, FontSize() / 10 dla fontu raylib).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Spacing(this Self& self, float spacing) {
        self.spacing_ = spacing;
        self.size_.reset();
        return self;
    }

    /** @brief Podaje odstęp między znakami (już z domyślną wartością). */
    float Spacing() const { return spacing_ < 0 ? Face().DefaultSpacing(fontSize_) : spacing_; }

    /**
     * @brief Ustawia odległość kolejnych wierszy.
     * @param multiplier Wielokrotność FontSize() (domyślnie 1.2).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& LineHeight(this Self& self, float multiplier) {
        self.lineHeight_ = multiplier;
        self.size_.reset();
        return self;
    }

    /** @brief Podaje odległość wierszy jako wielokrotność FontSize(). */
    float LineHeight() const { return lineHeight_; }

    /**
     * @brief Łamie wiersze dłuższe niż podana szerokość (na spacjach).
     * @details Słowo dłuższe niż cały wiersz jest dzielone między znakami. Przy łamaniu
     * wyrównanie Align() odnosi się do tej szerokości.
     * @param width Szerokość w pikselach; 0 - bez łamania (domyślnie).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& WrapWidth(this Self& self, float width) {
        self.wrapWidth_ = width;
        self.size_.reset();
        return self;
    }

    /** @brief Podaje szerokość łamania wierszy (0 - bez łamania). */
    float WrapWidth() const { return wrapWidth_; }

    /**
     * @brief Ustawia wyrównanie wierszy w bloku napisu.
     * @details Blok ma szerokość najdłuższego wiersza, a przy WrapWidth() - szerokość łamania.
     * @param align Wyrównanie (domyślnie TextAlign::Left).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Align(this Self& self, TextAlign align) {
        self.align_ = align;
        return self;
    }

    /** @brief Podaje wyrównanie wierszy. */
    TextAlign Align() const { return align_; }

    /**
     * @brief Ustawia czcionkę napisu.
     * @param typeface Czcionka (współdzielona) albo nullptr - z kontekstu albo domyślna.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Font(this Self& self, std::shared_ptr<const Typeface> typeface) {
        self.font_ = std::move(typeface);
        self.size_.reset();
        return self;
    }

    /** @brief Podaje czcionkę ustawioną na napisie (nullptr - z kontekstu albo domyślna). */
    const std::shared_ptr<const Typeface>& Font() const { return font_; }

    /**
     * @brief Ustawia przesunięcie punktu zaczepienia w pikselach (od lewego górnego rogu).
     * @details Pozycja obiektu wskazuje punkt Origin() + Anchor() * Size(), a obrót i skala
     * działają wokół niego - tak samo jak shapes::Sprite::Origin().
     * @param origin Punkt w pikselach napisu.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Origin(this Self& self, Vector2 origin) {
        self.origin_ = origin;
        return self;
    }

    /** @brief Podaje przesunięcie punktu zaczepienia w pikselach. */
    Vector2 Origin() const { return origin_; }

    /**
     * @brief Ustawia punkt zaczepienia jako część rozmiaru napisu.
     * @details {0, 0} - lewy górny róg, {0.5, 0.5} - środek, {1, 1} - prawy dolny róg. Punkt
     * przesuwa się razem ze zmianą treści, więc wyśrodkowany licznik zostaje na środku.
     * @param anchor Ułamek szerokości i wysokości.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Anchor(this Self& self, Vector2 anchor) {
        self.anchor_ = anchor;
        return self;
    }

    /** @brief Podaje punkt zaczepienia jako część rozmiaru napisu. */
    Vector2 Anchor() const { return anchor_; }

    /**
     * @brief Mierzy napis.
     * @details Wynik jest zapamiętywany do zmiany treści albo parametrów tekstu. Font
     * domyślny raylib jest dostępny dopiero po otwarciu okna - wcześniej pomiar napisu bez
     * wczytanej czcionki daje {0, 0}.
     * @return Szerokość i wysokość w pikselach (w układzie lokalnym).
     */
    Vector2 Size() const;

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    /** @brief Czcionka do użycia: własna, z kontekstu albo domyślna. */
    const Typeface& Face() const { return ResolveTypeface(font_, Context<DefaultTypeface>()); }

    /** @brief Parametry układu dla Typeface. */
    TextStyle Style() const;

    /** @brief Lewy górny róg napisu w układzie lokalnym (minus punkt zaczepienia). */
    Vector2 TopLeft() const;

    std::string content_;                  /**< Treść. */
    float fontSize_ = 20.0f;               /**< Wysokość tekstu. */
    float spacing_ = -1.0f;                /**< Odstęp między znakami; < 0 - domyślny. */
    float lineHeight_ = 1.2f;              /**< Odległość wierszy (wielokrotność wysokości). */
    float wrapWidth_ = 0.0f;               /**< Szerokość łamania; 0 - bez łamania. */
    TextAlign align_ = TextAlign::Left;    /**< Wyrównanie wierszy. */
    std::shared_ptr<const Typeface> font_; /**< Czcionka albo nullptr. */
    Vector2 origin_{0, 0};                 /**< Przesunięcie punktu zaczepienia (piksele). */
    Vector2 anchor_{0, 0};                 /**< Punkt zaczepienia (część rozmiaru). */
    mutable std::optional<Vector2> size_;  /**< Zapamiętany pomiar. */
    mutable const Typeface* measured_ = nullptr; /**< Czcionka zapamiętanego pomiaru. */
};

} // namespace shapes
