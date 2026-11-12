/**
 * @file Ellipse.cpp
 * @brief Rysowanie i test trafienia kształtu shapes::Ellipse.
 */
#include <raycade/shapes/Ellipse.hpp>

namespace shapes {

bool Ellipse::Contains(Vector2 point) const {
    const auto local = TryToLocal(point);
    if (!local || radius_.x == 0.0f || radius_.y == 0.0f) return false;
    const float x = local->x / radius_.x;
    const float y = local->y / radius_.y;
    return x * x + y * y <= 1.0f;
}

void Ellipse::Draw(PrimitiveRenderer& renderer) const {
    Brush(renderer).Ellipse({0.0f, 0.0f}, radius_);
}

void Ellipse::ConvexParts(std::vector<ConvexPart>& parts) const {
    parts.push_back(ConvexPart::Ellipse({0.0f, 0.0f}, radius_));
}

} // namespace shapes
