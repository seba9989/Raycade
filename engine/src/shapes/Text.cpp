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
    return local && ::CheckCollisionPointRec(*local, {0, 0, size.x, size.y});
}

void Text::Draw(PrimitiveRenderer&) const {
    Face().Draw(content_, {0, 0}, fontSize_, Spacing(), color_);
}

} // namespace shapes
