/**
 * @file Rect.cpp
 * @brief Rysowanie i test trafienia kształtu shapes::Rect.
 */
#include <raycade/shapes/Rect.hpp>

namespace shapes {

bool Rect::Contains(Vector2 point) const {
    const auto local = TryToLocal(point);
    return local && ::CheckCollisionPointRec(*local, {0.0f, 0.0f, size_.x, size_.y});
}

void Rect::Draw(PrimitiveRenderer& renderer) const {
    Brush(renderer).Rect({0.0f, 0.0f}, size_);
}

} // namespace shapes
