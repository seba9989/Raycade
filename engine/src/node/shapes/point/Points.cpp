/**
 * @file Points.cpp
 * @brief Rysowanie i test trafienia kształtu node::shapes::Points.
 */
#include <raycade/node/shapes/point/Points.hpp>

#include "../HitDistance.hpp"

namespace node::shapes {

bool Points::Contains(Vector2 point) const {
    for (const Point2D position : positions_)
        if (::CheckCollisionPointCircle(point, World(position), kHitDistance)) return true;
    return false;
}

void Points::Draw(PrimitiveRenderer& renderer) const {
    Brush(renderer).Points(positions_);
}

void Points::ConvexParts(std::vector<ConvexPart>& parts) const {
    for (const Point2D position : positions_)
        parts.push_back(ConvexPart::Point(position));
}

} // namespace node::shapes
