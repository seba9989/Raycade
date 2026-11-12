/**
 * @file Point.cpp
 * @brief Rysowanie i test trafienia kształtu shapes::Point.
 */
#include <raycade/shapes/Point.hpp>

#include "HitDistance.hpp"

namespace shapes {

bool Point::Contains(Vector2 point) const {
    return ::CheckCollisionPointCircle(point, WorldPosition(), kHitDistance);
}

void Point::Draw(PrimitiveRenderer& renderer) const {
    Brush(renderer).Point({0.0f, 0.0f});
}

} // namespace shapes
