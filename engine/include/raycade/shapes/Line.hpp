/**
 * @file Line.hpp
 * @brief Kształt Line - pojedynczy odcinek.
 */
#pragma once

#include <raycade/shapes/Shape.hpp>

namespace shapes {

/** @brief Odcinek, którego końce są określone względem pozycji obiektu. */
class Line : public Shape {
  public:
    /**
     * @brief Ustawia początek odcinka.
     * @param position Pozycja lokalna.
     * @return Ten sam odcinek.
     */
    template <typename Self> Self& From(this Self& self, Point2D position) {
        self.from_ = position;
        return self;
    }

    /**
     * @brief Ustawia koniec odcinka.
     * @param position Pozycja lokalna.
     * @return Ten sam odcinek.
     */
    template <typename Self> Self& To(this Self& self, Point2D position) {
        self.to_ = position;
        return self;
    }

    /** @brief Podaje początek odcinka (względem pozycji obiektu). */
    Point2D From() const { return from_; }

    /** @brief Podaje koniec odcinka (względem pozycji obiektu). */
    Point2D To() const { return to_; }

    /** @copydoc Entity::Contains */
    bool Contains(Vector2 point) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    Point2D from_; /**< Początek względem pozycji obiektu. */
    Point2D to_;   /**< Koniec względem pozycji obiektu. */
};

} // namespace shapes
