/**
 * @file Point.hpp
 * @brief Kształt Point - pojedynczy piksel.
 */
#pragma once

#include <raycade/shapes/Shape.hpp>

namespace shapes {

/** @brief Pojedynczy punkt rysowany w pozycji obiektu. */
class Point : public Shape {
  public:
    /** @copydoc Entity::Contains */
    bool Contains(Vector2 point) const override;

    /** @copydoc Shape::ConvexParts */
    void ConvexParts(std::vector<ConvexPart>& parts) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;
};

} // namespace shapes
