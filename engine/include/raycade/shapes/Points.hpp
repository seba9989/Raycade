/**
 * @file Points.hpp
 * @brief Kształt Points - zbiór punktów (wersja tablicowa).
 */
#pragma once

#include <utility>
#include <vector>

#include <raycade/shapes/Shape.hpp>

namespace shapes {

/** @brief Zbiór punktów, których pozycje są lokalne względem obiektu. */
class Points : public Shape {
  public:
    /**
     * @brief Ustawia pozycje punktów.
     * @param positions Pozycje lokalne.
     * @return Ten sam zbiór.
     */
    template <typename Self> Self& Positions(this Self& self, std::vector<Point2D> positions) {
        self.positions_ = std::move(positions);
        return self;
    }

    /** @brief Podaje pozycje punktów (względem pozycji obiektu). */
    const std::vector<Point2D>& Positions() const { return positions_; }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    std::vector<Point2D> positions_; /**< Pozycje punktów względem obiektu. */
};

} // namespace shapes
