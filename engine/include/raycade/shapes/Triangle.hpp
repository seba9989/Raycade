/**
 * @file Triangle.hpp
 * @brief Kształt Triangle - trójkąt.
 */
#pragma once

#include <array>

#include <raycade/entity/Entity.hpp>

namespace shapes {

/** @brief Trójkąt z wierzchołkami lokalnymi względem pozycji obiektu. */
class Triangle : public Entity {
  public:
    /**
     * @brief Ustawia trzy wierzchołki trójkąta.
     * @param a Pierwszy wierzchołek.
     * @param b Drugi wierzchołek.
     * @param c Trzeci wierzchołek.
     * @return Ten sam trójkąt.
     */
    template <typename Self> Self& Vertices(this Self& self, Point2D a, Point2D b, Point2D c) {
        self.a_ = a;
        self.b_ = b;
        self.c_ = c;
        return self;
    }

    /** @brief Podaje trzy wierzchołki. */
    std::array<Point2D, 3> Vertices() const { return {a_, b_, c_}; }

    /**
     * @brief Rysuje kontur własnym algorytmem przyrostowym zamiast raylib.
     * @details Nie zmienia wypełnienia (Fill()).
     * @param value true dla algorytmu własnego, false dla raylib.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Incremental(this Self& self, bool value = true) {
        self.incremental_ = value;
        return self;
    }

    /** @brief Sprawdza, czy kontur rysuje algorytm przyrostowy. */
    bool IsIncremental() const { return incremental_; }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        return ::CheckCollisionPointTriangle(point, World(a_), World(b_), World(c_));
    }

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Fill(filled_).Incremental(incremental_).Triangle(a_, b_, c_);
    }

  private:
    Point2D a_;                /**< Pierwszy wierzchołek. */
    Point2D b_;                /**< Drugi wierzchołek. */
    Point2D c_;                /**< Trzeci wierzchołek. */
    bool incremental_ = false; /**< Czy kontur rysuje algorytm przyrostowy. */
};

} // namespace shapes
