/**
 * @file Point.hpp
 * @brief Kształt Point - pojedynczy piksel.
 */
#pragma once

#include "../Entity.hpp"
#include "Line.hpp"

namespace shapes {

/** @brief Pojedynczy punkt rysowany w pozycji obiektu. */
class Point : public Entity {
  public:
    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        return ::CheckCollisionPointCircle(point, WorldPosition(), Line::kHitDistance);
    }

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Point({0.0f, 0.0f});
    }
};

} // namespace shapes
