/**
 * @file Polygon.cpp
 * @brief Rysowanie i test trafienia kształtu shapes::Polygon.
 */
#include <raycade/shapes/Polygon.hpp>

namespace shapes {

bool Polygon::Contains(Vector2 point) const {
    if (!simple_) return false;
    const std::vector<Vector2> world = WorldVertices(vertices_);
    return ::CheckCollisionPointPoly(point, world.data(), static_cast<int>(world.size()));
}

void Polygon::Draw(PrimitiveRenderer& renderer) const {
    Brush(renderer).Polygon(vertices_);
}

void Polygon::ConvexParts(std::vector<ConvexPart>& parts) const {
    if (!simple_) return;
    for (const std::vector<Point2D>& piece : ConvexPieces(vertices_))
        parts.push_back(ConvexPart::Polygon({piece.begin(), piece.end()}));
}

} // namespace shapes
