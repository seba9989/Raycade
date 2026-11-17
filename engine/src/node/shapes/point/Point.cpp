/**
 * @file Point.cpp
 * @brief Rysowanie i test trafienia kształtu node::shapes::Point.
 */
#include <raycade/node/shapes/point/Point.hpp>

#include "../HitDistance.hpp"

namespace node::shapes {

bool Point::Contains(Vector2 point) const {
    return ::CheckCollisionPointCircle(point, WorldPosition(), kHitDistance);
}

void Point::Draw(PrimitiveRenderer& renderer) const {
    Brush(renderer).Point({0.0f, 0.0f});
}

void Point::ConvexParts(std::vector<ConvexPart>& parts) const {
    parts.push_back(ConvexPart::Point({0.0f, 0.0f}));
}

} // namespace node::shapes
