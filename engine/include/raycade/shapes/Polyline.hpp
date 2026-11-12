/**
 * @file Polyline.hpp
 * @brief Kształt Polyline - łamana otwarta lub zamknięta.
 */
#pragma once

#include <vector>

#include <raycade/shapes/Shape.hpp>

namespace shapes {

/**
 * @brief Otwarta lub zamknięta łamana o lokalnych współrzędnych wierzchołków.
 * @details W przeciwieństwie do Polygon nie sprawdza samoprzecięć.
 */
class Polyline : public Shape {
  public:
    /**
     * @brief Ustawia wierzchołki łamanej.
     * @param vertices Punkty w kolejności.
     * @return Ta sama łamana.
     */
    template <typename Self> Self& Vertices(this Self& self, std::vector<Point2D> vertices) {
        self.vertices_ = std::move(vertices);
        return self;
    }

    /**
     * @brief Ustawia domknięcie łamanej.
     * @param value Czy połączyć ostatni punkt z pierwszym.
     * @return Ta sama łamana.
     */
    template <typename Self> Self& Closed(this Self& self, bool value = true) {
        self.closed_ = value;
        return self;
    }

    /** @brief Podaje wierzchołki łamanej. */
    const std::vector<Point2D>& Vertices() const { return vertices_; }

    /** @brief Sprawdza, czy łamana jest zamknięta. */
    bool IsClosed() const { return closed_; }

    /**
     * @copydoc Entity::Contains
     * @details Łamana zamknięta jest traktowana jak wielokąt, otwarta - jak zbiór odcinków.
     */
    bool Contains(Vector2 point) const override;

    /** @copydoc Shape::ConvexParts */
    void ConvexParts(std::vector<ConvexPart>& parts) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    std::vector<Point2D> vertices_; /**< Wierzchołki względem pozycji obiektu. */
    bool closed_ = false;           /**< Czy łamana jest zamknięta. */
};

} // namespace shapes
