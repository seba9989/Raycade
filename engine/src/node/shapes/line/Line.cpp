/**
 * @file Line.cpp
 * @brief Rysowanie i test trafienia kształtu node::shapes::Line.
 */
#include <raycade/node/shapes/line/Line.hpp>

#include "../HitDistance.hpp"

namespace node::shapes {

bool Line::Contains(Vector2 point) const {
    return ::CheckCollisionPointLine(point, World(from_), World(to_), kHitDistance);
}

void Line::Draw(PrimitiveRenderer& renderer) const {
    Brush(renderer).Line(from_, to_);
}

void Line::ConvexParts(std::vector<ConvexPart>& parts) const {
    parts.push_back(ConvexPart::Segment(from_, to_));
}

} // namespace node::shapes
