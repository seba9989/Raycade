/**
 * @file Points.hpp
 * @brief Kształt Points - zbiór punktów (wersja tablicowa).
 */
#pragma once

#include <utility>
#include <vector>

#include "../Entity.hpp"
#include "Line.hpp"

namespace shapes {

/** @brief Zbiór punktów, których pozycje są lokalne względem obiektu. */
class Points : public Entity {
  public:
    /**
     * @brief Ustawia pozycje punktów.
     * @param positions Pozycje lokalne.
     * @return Ten sam zbiór.
     */
    Points& Positions(std::vector<Vector2> positions) {
        positions_ = std::move(positions);
        return *this;
    }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        for (const Vector2& position : positions_)
            if (::CheckCollisionPointCircle(point, World(position), Line::kHitDistance))
                return true;
        return false;
    }

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Points(positions_);
    }

  private:
    std::vector<Vector2> positions_; /**< Pozycje punktów względem obiektu. */
};

} // namespace shapes
