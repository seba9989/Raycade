/**
 * @file Canvas.cpp
 * @brief Płótno: rysowanie po bitmapie, test trafienia i wyświetlanie.
 */
#include <raycade/shapes/Canvas.hpp>

namespace shapes {

Canvas& Canvas::Content(const std::function<void(PrimitiveRenderer&)>& paint) {
    PrimitiveRenderer renderer;
    renderer.Target(bitmap_);
    paint(renderer);
    return *this;
}

bool Canvas::Contains(Vector2 point) const {
    const auto local = TryToLocal(point);
    return local &&
           ::CheckCollisionPointRec(*local, {0.0f, 0.0f, static_cast<float>(bitmap_.Width()),
                                             static_cast<float>(bitmap_.Height())});
}

void Canvas::Draw(PrimitiveRenderer& renderer) const {
    if (bitmap_.Width() > 0) renderer.Blit(bitmap_, {0.0f, 0.0f});
}

} // namespace shapes
