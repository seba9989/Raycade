/**
 * @file Circle.hpp
 * @brief Kształt Circle - okrąg lub koło.
 */
#pragma once

#include <raycade/shapes/Shape.hpp>

namespace shapes {

/** @brief Okrąg lub koło; pozycja obiektu oznacza środek. */
class Circle : public Shape {
  public:
    /**
     * @brief Ustawia promień.
     * @param radius Promień w pikselach.
     * @return Ten sam okrąg.
     */
    template <typename Self> Self& Radius(this Self& self, float radius) {
        self.radius_ = radius;
        return self;
    }

    /** @brief Podaje promień. */
    float Radius() const { return radius_; }

    /** @copydoc Entity::Contains */
    bool Contains(Vector2 point) const override;

    /** @copydoc Shape::ConvexParts */
    void ConvexParts(std::vector<ConvexPart>& parts) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    float radius_ = 10.0f; /**< Promień w pikselach. */
};

} // namespace shapes
