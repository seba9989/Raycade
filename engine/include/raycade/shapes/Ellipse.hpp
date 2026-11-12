/**
 * @file Ellipse.hpp
 * @brief Kształt Ellipse - elipsa pusta lub wypełniona.
 */
#pragma once

#include <raycade/shapes/Shape.hpp>

namespace shapes {

/** @brief Elipsa; pozycja obiektu oznacza środek, a promienie obie półosie. */
class Ellipse : public Shape {
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

    /** @brief Podaje promienie {poziomy, pionowy}. */
    Vector2 Radius() const { return radius_; }

    /** @copydoc Entity::Contains */
    bool Contains(Vector2 point) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    Vector2 radius_{20, 10}; /**< Promienie {poziomy, pionowy}. */
};

} // namespace shapes
