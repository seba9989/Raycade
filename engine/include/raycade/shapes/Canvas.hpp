/**
 * @file Canvas.hpp
 * @brief Kształt Canvas - płótno z bitmapą w RAM do rysowania i wypełniania.
 */
#pragma once

#include <functional>

#include <raycade/entity/Entity.hpp>
#include <raycade/render/Bitmap.hpp>

namespace shapes {

/**
 * @brief Płótno: bitmapa w RAM wyświetlana w pozycji obiektu (lewy górny róg).
 * @details Po płótnie można rysować i wypełniać obszary (BoundaryFill / FloodFill), bo
 * w odróżnieniu od ekranu da się z niego odczytać kolory pikseli. Współrzędne w Edit()
 * są liczone od lewego górnego rogu płótna.
 * @code{.cpp}
 * scene.Spawn<shapes::Canvas>().Position({50, 50}).Size({300, 200}).Background(WHITE)
 *     .Edit([](PrimitiveRenderer& r) { r.Paint(BLACK).Circle({150, 100}, 80); });
 * @endcode
 */
class Canvas : public Entity {
  public:
    /**
     * @brief Puste płótno z kolorem mnożącym WHITE (piksele bez zmian).
     * @details Paint() działa tu jak w shapes::Sprite i shapes::Surface: mnoży piksele
     * bitmapy przy wyświetlaniu, a samej bitmapy nie zmienia.
     */
    Canvas() { Paint(WHITE); }

    /**
     * @brief Ustawia rozmiar, tworząc bitmapę od nowa.
     * @param size Szerokość i wysokość w pikselach.
     * @return To samo płótno.
     */
    template <typename Self> Self& Size(this Self& self, Vector2 size) {
        self.bitmap_ = Bitmap{static_cast<int>(size.x), static_cast<int>(size.y), self.background_};
        return self;
    }

    /**
     * @brief Ustawia kolor tła i czyści nim bitmapę.
     * @param color Kolor tła.
     * @return To samo płótno.
     */
    template <typename Self> Self& Background(this Self& self, Color color) {
        self.background_ = color;
        self.bitmap_.Clear(color);
        return self;
    }

    /**
     * @brief Podaje kolor tła.
     * @return Kolor używany przez Clear().
     */
    Color Background() const { return background_; }

    /**
     * @brief Czyści płótno bieżącym kolorem tła.
     * @return To samo płótno.
     */
    template <typename Self> Self& Clear(this Self& self) {
        self.bitmap_.Clear(self.background_);
        return self;
    }

    /**
     * @brief Rysuje po bitmapie płótna (zmienia jej piksele na stałe).
     * @details Wcześniej Content(); nazwa Content() oznacza w silniku treść tekstu
     * (shapes::Text, ui::TextField).
     * @param paint Funkcja otrzymująca renderer z celem ustawionym już na bitmapę płótna.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& Edit(this Self& self, const std::function<void(PrimitiveRenderer&)>& paint) {
        self.EditPixels(paint);
        return self;
    }

    /**
     * @brief Udostępnia bitmapę płótna.
     * @return Referencja do bitmapy.
     */
    Bitmap& Pixels() { return bitmap_; }

    /** @brief Podaje rozmiar bitmapy płótna. */
    Vector2 Size() const {
        return {static_cast<float>(bitmap_.Width()), static_cast<float>(bitmap_.Height())};
    }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    /** @brief Wspólna część Edit() - rysowanie rendererem z celem na bitmapie płótna. */
    void EditPixels(const std::function<void(PrimitiveRenderer&)>& paint);

    Bitmap bitmap_;            /**< Zawartość płótna. */
    Color background_ = WHITE; /**< Kolor używany przez Clear(). */
};

} // namespace shapes
