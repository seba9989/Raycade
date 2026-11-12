/**
 * @file Polyline.cpp
 * @brief Rysowanie i test trafienia kształtu shapes::Polyline.
 */
#include <raycade/shapes/Polyline.hpp>

#include "HitDistance.hpp"

namespace shapes {

bool Polyline::Contains(Vector2 point) const {
    const std::vector<Vector2> world = WorldVertices(vertices_);
    const int count = static_cast<int>(world.size());
    if (closed_ && count >= 3) return ::CheckCollisionPointPoly(point, world.data(), count);
    for (int i = 0; i + 1 < count; ++i)
        if (::CheckCollisionPointLine(point, world[i], world[i + 1], kHitDistance)) return true;
    return false;
}

void Polyline::Draw(PrimitiveRenderer& renderer) const {
    Brush(renderer).Polyline(vertices_, closed_);
}

} // namespace shapes
