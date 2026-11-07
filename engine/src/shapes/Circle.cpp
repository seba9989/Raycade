/**
 * @file Circle.cpp
 * @brief Rysowanie i test trafienia kształtu shapes::Circle.
 */
#include <raycade/shapes/Circle.hpp>

namespace shapes {

bool Circle::Contains(Vector2 point) const {
    const auto local = TryToLocal(point);
    return local && ::CheckCollisionPointCircle(*local, {0.0f, 0.0f}, radius_);
}

void Circle::Draw(PrimitiveRenderer& renderer) const {
    renderer.Paint(color_).Fill(filled_).Symmetric(symmetric_).Circle({0.0f, 0.0f}, radius_);
}

} // namespace shapes
