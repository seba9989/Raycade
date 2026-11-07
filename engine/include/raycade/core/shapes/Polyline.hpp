/**
 * @file Polyline.hpp
 * @brief Kształt Polyline - łamana otwarta lub zamknięta.
 */
#pragma once

#include <vector>

#include "../Entity.hpp"
#include "Line.hpp"

namespace shapes {

/**
 * @brief Otwarta lub zamknięta łamana o lokalnych współrzędnych wierzchołków.
 * @details W przeciwieństwie do Polygon nie sprawdza samoprzecięć.
 */
class Polyline : public Entity {
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

    /**
     * @brief Wybiera własny algorytm przyrostowy dla odcinków.
     * @param value Czy włączyć algorytm własny.
     * @return Ta sama łamana.
     */
    template <typename Self> Self& Incremental(this Self& self, bool value = true) {
        self.incremental_ = value;
        return self;
    }

    /** @brief Podaje wierzchołki łamanej. */
    const std::vector<Point2D>& Vertices() const { return vertices_; }

    /** @brief Sprawdza, czy łamana jest zamknięta. */
    bool IsClosed() const { return closed_; }

    /** @brief Sprawdza, czy odcinki rysuje algorytm przyrostowy. */
    bool IsIncremental() const { return incremental_; }

    /**
     * @copydoc Updatable::Contains
     * @details Łamana zamknięta jest traktowana jak wielokąt, otwarta - jak zbiór odcinków.
     */
    bool Contains(Vector2 point) const override {
        const std::vector<Vector2> world = WorldVertices(vertices_);
        const int count = static_cast<int>(world.size());
        if (closed_ && count >= 3) return ::CheckCollisionPointPoly(point, world.data(), count);
        for (int i = 0; i + 1 < count; ++i)
            if (::CheckCollisionPointLine(point, world[i], world[i + 1], Line::kHitDistance))
                return true;
        return false;
    }

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Incremental(incremental_).Polyline(vertices_, closed_);
    }

  private:
    std::vector<Point2D> vertices_; /**< Wierzchołki względem pozycji obiektu. */
    bool closed_ = false;           /**< Czy łamana jest zamknięta. */
    bool incremental_ = false;      /**< Czy użyć algorytmu przyrostowego. */
};

} // namespace shapes
