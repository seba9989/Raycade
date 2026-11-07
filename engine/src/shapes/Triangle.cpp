/**
 * @file Triangle.cpp
 * @brief Rysowanie i test trafienia kształtu shapes::Triangle.
 */
#include <raycade/shapes/Triangle.hpp>

namespace shapes {

bool Triangle::Contains(Vector2 point) const {
    return ::CheckCollisionPointTriangle(point, World(a_), World(b_), World(c_));
}

void Triangle::Draw(PrimitiveRenderer& renderer) const {
    renderer.Paint(color_).Fill(filled_).Incremental(incremental_).Triangle(a_, b_, c_);
}

} // namespace shapes
