/**
 * @file Polygon.hpp
 * @brief Kształt Polygon - wielokąt (łamana zwyczajna zamknięta).
 */
#pragma once

#include <vector>

#include "../../render/Geometry.hpp"
#include "../Entity.hpp"

namespace shapes {

/**
 * @brief Wielokąt (łamana zwyczajna zamknięta) o wierzchołkach lokalnych względem obiektu.
 * @details Łamana wiązana (przecinające się krawędzie niesąsiednie) nie jest rysowana -
 * można to sprawdzić przez Simple(). Fill() wypełnia wielokąt algorytmem scanline
 * z kontrolą parzystości.
 */
class Polygon : public Entity {
  public:
    /**
     * @brief Ustawia wierzchołki obwodu.
     * @param vertices Punkty w kolejności obwodu.
     * @return Ten sam wielokąt.
     */
    template <typename Self> Self& Vertices(this Self& self, std::vector<Point2D> vertices) {
        self.vertices_ = std::move(vertices);
        return self;
    }

    /**
     * @brief Buduje wielokąt z uporządkowanego, zamkniętego łańcucha odcinków.
     * @param edges Odcinki stykające się kolejno końcami i domknięte.
     * @return Ten sam wielokąt; dla niepoprawnego łańcucha - bez wierzchołków.
     */
    template <typename Self> Self& Edges(this Self& self, const std::vector<LineSegment>& edges) {
        self.vertices_ = PolygonVertices(edges).value_or(std::vector<Point2D>{});
        return self;
    }

    /**
     * @brief Wybiera algorytm przyrostowy dla konturu.
     * @param value Czy włączyć algorytm własny.
     * @return Ten sam wielokąt.
     */
    template <typename Self> Self& Incremental(this Self& self, bool value = true) {
        self.incremental_ = value;
        return self;
    }

    /**
     * @brief Sprawdza, czy wielokąt jest łamaną zwyczajną.
     * @return Wynik IsSimplePolygon() dla bieżących wierzchołków.
     */
    bool Simple() const { return IsSimplePolygon(vertices_); }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        if (!Simple()) return false;
        std::vector<Vector2> world;
        for (const Point2D& vertex : WorldVertices(vertices_))
            world.push_back(vertex);
        return ::CheckCollisionPointPoly(point, world.data(), static_cast<int>(world.size()));
    }

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Fill(filled_).Incremental(incremental_).Polygon(vertices_);
    }

  private:
    std::vector<Point2D> vertices_; /**< Wierzchołki względem pozycji obiektu. */
    bool incremental_ = false;      /**< Czy użyć algorytmu przyrostowego. */
};

} // namespace shapes
