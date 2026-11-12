/**
 * @file Text.cpp
 * @brief Pomiar, test trafienia i rysowanie napisu.
 */
#include <raycade/visual/Text.hpp>

namespace visual {

TextStyle Text::Style() const {
    TextStyle style;
    style.size = fontSize_;
    style.spacing = Spacing();
    style.lineHeight = lineHeight_;
    style.wrapWidth = wrapWidth_;
    style.align = align_;
    return style;
}

Vector2 Text::Size() const {
    const Typeface& typeface = Face();
    if (typeface.IsDefault() && !::IsWindowReady()) return {0, 0};
    // czcionka z kontekstu mogła się zmienić (inny rodzic, nowy DefaultTypeface)
    if (size_ && measured_ == &typeface) return *size_;
    size_ = typeface.Measure(content_, Style());
    measured_ = &typeface;
    return *size_;
}

Vector2 Text::Pivot() const {
    if (anchor_.x == 0.0f && anchor_.y == 0.0f) return origin_; // bez pomiaru tekstu
    const Vector2 size = Size();
    return {origin_.x + anchor_.x * size.x, origin_.y + anchor_.y * size.y};
}

Vector2 Text::TopLeft() const {
    const Vector2 pivot = Pivot();
    return {-pivot.x, -pivot.y};
}

bool Text::Contains(Vector2 point) const {
    const auto local = TryToLocal(point);
    if (!local) return false;
    const Vector2 corner = TopLeft();
    const Vector2 size = Size();
    return ::CheckCollisionPointRec(*local, {corner.x, corner.y, size.x, size.y});
}

void Text::Draw(PrimitiveRenderer&) const {
    Face().Draw(content_, TopLeft(), Style(), color_);
}

} // namespace visual
