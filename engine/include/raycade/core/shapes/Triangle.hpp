/**
 * @file Triangle.hpp
 * @brief Kształt Triangle - trójkąt.
 */
#pragma once

#include "../Entity.hpp"

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

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        return ::CheckCollisionPointTriangle(point, World(a_), World(b_), World(c_));
    }

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Fill(filled_).Triangle(a_, b_, c_);
    }

  private:
    Point2D a_; /**< Pierwszy wierzchołek. */
    Point2D b_; /**< Drugi wierzchołek. */
    Point2D c_; /**< Trzeci wierzchołek. */
};

} // namespace shapes
