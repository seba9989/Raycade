/**
 * @file Canvas.cpp
 * @brief Płótno: rysowanie po bitmapie, test trafienia i wyświetlanie.
 */
#include <raycade/visual/Canvas.hpp>

#include <algorithm>
#include <utility>

namespace visual {

void Canvas::EditPixels(const std::function<void(PrimitiveRenderer&)>& paint) {
    PrimitiveRenderer renderer;
    renderer.Target(bitmap_);
    paint(renderer);
}

void Canvas::Resize(Vector2 size) {
    Bitmap next{static_cast<int>(size.x), static_cast<int>(size.y), background_};
    const int width = std::min(next.Width(), bitmap_.Width());
    const int height = std::min(next.Height(), bitmap_.Height());
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            next.Pixel(x, y, bitmap_.Pixel(x, y));
    bitmap_ = std::move(next);
}

bool Canvas::Contains(Vector2 point) const {
    const auto local = TryToLocal(point);
    return local && ::CheckCollisionPointRec(
                        *local, {0.0f, 0.0f, static_cast<float>(bitmap_.Width()),
                                 static_cast<float>(bitmap_.Height())}
                    );
}

void Canvas::Draw(PrimitiveRenderer& renderer) const {
    if (bitmap_.Width() > 0) renderer.Blit(bitmap_, {0.0f, 0.0f}, color_);
}

} // namespace visual
