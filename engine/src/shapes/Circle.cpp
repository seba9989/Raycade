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
    Brush(renderer).Circle({0.0f, 0.0f}, radius_);
}

void Circle::ConvexParts(std::vector<ConvexPart>& parts) const {
    parts.push_back(ConvexPart::Ellipse({0.0f, 0.0f}, {radius_, radius_}));
}

} // namespace shapes
