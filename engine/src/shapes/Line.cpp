/**
 * @file Line.cpp
 * @brief Rysowanie i test trafienia kształtu shapes::Line.
 */
#include <raycade/shapes/Line.hpp>

#include "HitDistance.hpp"

namespace shapes {

bool Line::Contains(Vector2 point) const {
    return ::CheckCollisionPointLine(point, World(from_), World(to_), kHitDistance);
}

void Line::Draw(PrimitiveRenderer& renderer) const {
    Brush(renderer).Line(from_, to_);
}

} // namespace shapes
