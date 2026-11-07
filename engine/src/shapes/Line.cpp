/**
 * @file Line.cpp
 * @brief Rysowanie i test trafienia kształtu shapes::Line.
 */
#include <raycade/shapes/Line.hpp>

namespace shapes {

bool Line::Contains(Vector2 point) const {
    return ::CheckCollisionPointLine(point, World(from_), World(to_), kHitDistance);
}

void Line::Draw(PrimitiveRenderer& renderer) const {
    renderer.Paint(color_).Incremental(incremental_).Line(from_, to_);
}

} // namespace shapes
