/**
 * @file Text.hpp
 * @brief Kształt Text - napis z wybraną czcionką.
 */
#pragma once

#include <memory>
#include <string>
#include <utility>

#include <raycade/entity/Entity.hpp>
#include <raycade/render/Typeface.hpp>

namespace shapes {

/**
 * @brief Napis (także wielowierszowy, z '\n'), którego pozycja oznacza lewy górny róg.
 * @details W odróżnieniu od ::DrawText() w OnDraw() napis jest obiektem sceny: przesuwa się
 * z rodzicem i kamerą, obraca, skaluje, ma warstwę i reaguje na mysz w swoim prostokącie.
 * Kolor ustawia Paint(). Czcionkę wybiera Font(); bez niej napis używa DefaultTypeface
 * z kontekstu, a na końcu fontu raylib (bez polskich znaków - patrz Typeface).
 * @code{.cpp}
 * scene.Spawn<shapes::Text>()
 *     .Content("Wynik: 0")
 *     .FontSize(24)
 *     .Paint(DARKGRAY)
 *     .Position({20, 20})
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
        static_cast<Text&>(self).content_ = std::move(content);
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
        static_cast<Text&>(self).fontSize_ = size;
        return self;
    }

    /** @brief Podaje wysokość tekstu. */
    float FontSize() const { return fontSize_; }

    /**
     * @brief Ustawia odstęp między znakami.
     * @param spacing Odstęp w pikselach; ujemna wartość - domyślny (FontSize() / 10).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Spacing(this Self& self, float spacing) {
        static_cast<Text&>(self).spacing_ = spacing;
        return self;
    }

    /** @brief Podaje odstęp między znakami (już z domyślną wartością). */
    float Spacing() const { return spacing_ < 0 ? Typeface::DefaultSpacing(fontSize_) : spacing_; }

    /**
     * @brief Ustawia czcionkę napisu.
     * @param typeface Czcionka (współdzielona) albo nullptr - z kontekstu albo domyślna.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Font(this Self& self, std::shared_ptr<const Typeface> typeface) {
        static_cast<Text&>(self).font_ = std::move(typeface);
        return self;
    }

    /** @brief Podaje czcionkę ustawioną na napisie (nullptr - z kontekstu albo domyślna). */
    const std::shared_ptr<const Typeface>& Font() const { return font_; }

    /**
     * @brief Mierzy napis.
     * @details Font domyślny raylib jest dostępny dopiero po otwarciu okna - wcześniej
     * pomiar napisu bez wczytanej czcionki daje {0, 0}.
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

    std::string content_;                  /**< Treść. */
    float fontSize_ = 20.0f;               /**< Wysokość tekstu. */
    float spacing_ = -1.0f;                /**< Odstęp między znakami; < 0 - domyślny. */
    std::shared_ptr<const Typeface> font_; /**< Czcionka albo nullptr. */
};

} // namespace shapes
