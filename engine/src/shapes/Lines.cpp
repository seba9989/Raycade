/**
 * @file Lines.cpp
 * @brief Rysowanie i test trafienia kształtu shapes::Lines.
 */
#include <raycade/shapes/Lines.hpp>

#include "HitDistance.hpp"

namespace shapes {

bool Lines::Contains(Vector2 point) const {
    for (std::size_t i = 0; i + 1 < vertices_.size(); i += 2)
        if (::CheckCollisionPointLine(
                point, World(vertices_[i]), World(vertices_[i + 1]), kHitDistance
            ))
            return true;
    return false;
}

void Lines::Draw(PrimitiveRenderer& renderer) const {
    Brush(renderer).Lines(vertices_);
}

void Lines::ConvexParts(std::vector<ConvexPart>& parts) const {
    for (std::size_t i = 0; i + 1 < vertices_.size(); i += 2)
        parts.push_back(ConvexPart::Segment(vertices_[i], vertices_[i + 1]));
}

} // namespace shapes
