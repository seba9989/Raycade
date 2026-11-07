/**
 * @file Point.hpp
 * @brief Kształt Point - pojedynczy piksel.
 */
#pragma once

#include <raycade/entity/Entity.hpp>
#include <raycade/shapes/HitDistance.hpp>

namespace shapes {

/** @brief Pojedynczy punkt rysowany w pozycji obiektu. */
class Point : public Entity {
  public:
    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;
};

} // namespace shapes
