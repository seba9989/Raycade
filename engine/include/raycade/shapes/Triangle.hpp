/**
 * @file Triangle.hpp
 * @brief Kształt Triangle - trójkąt.
 */
#pragma once

#include <array>
#include <vector>

#include <raycade/shapes/Polygon.hpp>

namespace shapes {

/**
 * @brief Trójkąt z wierzchołkami lokalnymi względem pozycji obiektu.
 * @details Wielokąt o trzech wierzchołkach - test trafienia (i w przyszłości kolizje) dziedziczy
 * po Polygon, a rysuje się funkcjami trójkąta z PrimitiveRenderer. Setter Vertices() przyjmuje
 * dokładnie trzy punkty, a Edges() z Polygon jest niedostępne, więc trójkąt nie może dostać
 * innej liczby wierzchołków.
 */
class Triangle : public Polygon {
  public:
    /**
     * @brief Ustawia trzy wierzchołki trójkąta.
     * @param a Pierwszy wierzchołek.
     * @param b Drugi wierzchołek.
     * @param c Trzeci wierzchołek.
     * @return Ten sam trójkąt.
     */
    template <typename Self> Self& Vertices(this Self& self, Point2D a, Point2D b, Point2D c) {
        return self.Polygon::Vertices(std::vector<Point2D>{a, b, c});
    }

    /** @brief Podaje trzy wierzchołki. */
    std::array<Point2D, 3> Vertices() const;

    /** @brief Trójkąt ma zawsze trzy wierzchołki - budowanie z krawędzi jest wyłączone. */
    template <typename Self> Self& Edges(this Self& self, const std::vector<LineSegment>&) = delete;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;
};

} // namespace shapes
