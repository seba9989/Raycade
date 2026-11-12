/**
 * @file Polygon.hpp
 * @brief Kształt Polygon - wielokąt (łamana zwyczajna zamknięta).
 */
#pragma once

#include <vector>

#include <raycade/math/Geometry.hpp>
#include <raycade/shapes/Shape.hpp>

namespace shapes {

/**
 * @brief Wielokąt (łamana zwyczajna zamknięta) o wierzchołkach lokalnych względem obiektu.
 * @details Łamana wiązana (przecinające się krawędzie niesąsiednie) nie jest rysowana -
 * można to sprawdzić przez IsSimple(). Fill() wypełnia wielokąt algorytmem scanline
 * z kontrolą parzystości.
 */
class Polygon : public Shape {
  public:
    /**
     * @brief Ustawia wierzchołki obwodu.
     * @param vertices Punkty w kolejności obwodu.
     * @return Ten sam wielokąt.
     */
    template <typename Self> Self& Vertices(this Self& self, std::vector<Point2D> vertices) {
        self.vertices_ = std::move(vertices);
        self.simple_ = IsSimplePolygon(self.vertices_);
        return self;
    }

    /**
     * @brief Buduje wielokąt z uporządkowanego, zamkniętego łańcucha odcinków.
     * @param edges Odcinki stykające się kolejno końcami i domknięte.
     * @return Ten sam wielokąt; dla niepoprawnego łańcucha - bez wierzchołków.
     */
    template <typename Self> Self& Edges(this Self& self, const std::vector<LineSegment>& edges) {
        self.vertices_ = PolygonVertices(edges).value_or(std::vector<Point2D>{});
        self.simple_ = IsSimplePolygon(self.vertices_);
        return self;
    }

    /**
     * @brief Sprawdza, czy wielokąt jest łamaną zwyczajną.
     * @details Wynik jest liczony raz, przy ustawieniu wierzchołków, a nie przy każdym teście
     * trafienia.
     * @return Wynik IsSimplePolygon() dla bieżących wierzchołków.
     */
    bool IsSimple() const { return simple_; }

    /** @brief Podaje wierzchołki wielokąta. */
    const std::vector<Point2D>& Vertices() const { return vertices_; }

    /** @copydoc Entity::Contains */
    bool Contains(Vector2 point) const override;

    /** @copydoc Shape::ConvexParts */
    void ConvexParts(std::vector<ConvexPart>& parts) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    std::vector<Point2D> vertices_; /**< Wierzchołki względem pozycji obiektu. */
    bool simple_ = false;           /**< Zapamiętany wynik IsSimplePolygon(). */
};

} // namespace shapes
