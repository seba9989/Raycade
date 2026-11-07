/**
 * @file Points.cpp
 * @brief Rysowanie i test trafienia kształtu shapes::Points.
 */
#include <raycade/shapes/Points.hpp>

namespace shapes {

bool Points::Contains(Vector2 point) const {
    for (const Point2D position : positions_)
        if (::CheckCollisionPointCircle(point, World(position), kHitDistance)) return true;
    return false;
}

void Points::Draw(PrimitiveRenderer& renderer) const {
    renderer.Paint(color_).Points(positions_);
}

} // namespace shapes
