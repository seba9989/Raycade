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
 * w odróżnieniu od ekranu da się z niego odczytać kolory pikseli. Współrzędne w Content()
 * są liczone od lewego górnego rogu płótna.
 * @code{.cpp}
 * scene.Spawn<shapes::Canvas>().Position({50, 50}).Size({300, 200}).Background(WHITE)
 *     .Content([](PrimitiveRenderer& r) { r.Paint(BLACK).Circle({150, 100}, 80); });
 * @endcode
 */
class Canvas : public Entity {
  public:
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
     * @brief Wykonuje rysowanie na bitmapie płótna.
     * @param paint Funkcja otrzymująca renderer z celem ustawionym już na bitmapę płótna.
     * @return To samo płótno.
     */
    Canvas& Content(const std::function<void(PrimitiveRenderer&)>& paint) {
        PrimitiveRenderer renderer;
        renderer.Target(bitmap_);
        paint(renderer);
        return *this;
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
    bool Contains(Vector2 point) const override {
        const auto local = TryToLocal(point);
        return local &&
               ::CheckCollisionPointRec(*local, {0.0f, 0.0f, static_cast<float>(bitmap_.Width()),
                                                 static_cast<float>(bitmap_.Height())});
    }

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        if (bitmap_.Width() > 0) renderer.Blit(bitmap_, {0.0f, 0.0f});
    }

  private:
    Bitmap bitmap_;            /**< Zawartość płótna. */
    Color background_ = WHITE; /**< Kolor używany przez Clear(). */
};

} // namespace shapes
