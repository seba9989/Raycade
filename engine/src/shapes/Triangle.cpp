/**
 * @file Triangle.cpp
 * @brief Rysowanie kształtu shapes::Triangle.
 */
#include <raycade/shapes/Triangle.hpp>

namespace shapes {

std::array<Point2D, 3> Triangle::Vertices() const {
    const std::vector<Point2D>& vertices = Polygon::Vertices();
    if (vertices.size() != 3) return {};
    return {vertices[0], vertices[1], vertices[2]};
}

void Triangle::Draw(PrimitiveRenderer& renderer) const {
    const std::array<Point2D, 3> v = Vertices();
    Brush(renderer).Triangle(v[0], v[1], v[2]);
}

} // namespace shapes
