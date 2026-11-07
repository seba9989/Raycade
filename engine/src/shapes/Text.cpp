/**
 * @file Text.cpp
 * @brief Pomiar, test trafienia i rysowanie napisu.
 */
#include <raycade/shapes/Text.hpp>

namespace shapes {

Vector2 Text::Size() const {
    const Typeface& typeface = Face();
    if (typeface.IsDefault() && !::IsWindowReady()) return {0, 0};
    return typeface.Measure(content_, fontSize_, Spacing());
}

bool Text::Contains(Vector2 point) const {
    const auto local = TryToLocal(point);
    const Vector2 size = Size();
    return local && ::CheckCollisionPointRec(*local, {-origin_.x, -origin_.y, size.x, size.y});
}

void Text::Draw(PrimitiveRenderer&) const {
    Face().Draw(content_, {-origin_.x, -origin_.y}, fontSize_, Spacing(), color_);
}

} // namespace shapes
