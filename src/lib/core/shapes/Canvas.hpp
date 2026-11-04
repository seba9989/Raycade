/**
 * @file Canvas.hpp
 * @brief Kształt Canvas - płótno z bitmapą w RAM do rysowania i wypełniania.
 */
#pragma once

#include <functional>

#include "../../render/Bitmap.hpp"
#include "../Entity.hpp"

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
    Canvas& Size(Vector2 size) {
        bitmap_ = Bitmap{static_cast<int>(size.x), static_cast<int>(size.y), background_};
        return *this;
    }

    /**
     * @brief Ustawia kolor tła i czyści nim bitmapę.
     * @param color Kolor tła.
     * @return To samo płótno.
     */
    Canvas& Background(Color color) {
        background_ = color;
        bitmap_.Clear(color);
        return *this;
    }

    /**
     * @brief Czyści płótno bieżącym kolorem tła.
     * @return To samo płótno.
     */
    Canvas& Clear() {
        bitmap_.Clear(background_);
        return *this;
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
