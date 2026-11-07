/**
 * @file Ellipse.hpp
 * @brief Kształt Ellipse - elipsa pusta lub wypełniona.
 */
#pragma once

#include "../Entity.hpp"

namespace shapes {

/** @brief Elipsa; pozycja obiektu oznacza środek, a promienie obie półosie. */
class Ellipse : public Entity {
  public:
    /**
     * @brief Ustawia promienie poziomy i pionowy.
     * @param radius Wektor {promień poziomy, promień pionowy}.
     * @return Ta sama elipsa.
     */
    template <typename Self> Self& Radius(this Self& self, Vector2 radius) {
        self.radius_ = radius;
        return self;
    }

    /**
     * @brief Wybiera własny algorytm z czterokrotną symetrią zamiast raylib.
     * @param value true dla algorytmu własnego, false dla raylib.
     * @return Ta sama elipsa.
     */
    template <typename Self> Self& Symmetric(this Self& self, bool value = true) {
        self.symmetric_ = value;
        return self;
    }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        const auto local = TryToLocal(point);
        if (!local || radius_.x == 0.0f || radius_.y == 0.0f) return false;
        const float x = local->x / radius_.x;
        const float y = local->y / radius_.y;
        return x * x + y * y <= 1.0f;
    }

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Fill(filled_).Symmetric(symmetric_).Ellipse({0.0f, 0.0f}, radius_);
    }

  private:
    Vector2 radius_{20, 10}; /**< Promienie {poziomy, pionowy}. */
    bool symmetric_ = false; /**< Czy użyć własnego algorytmu. */
};

} // namespace shapes
