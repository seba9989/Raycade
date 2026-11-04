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
    Triangle& Vertices(Vector2 a, Vector2 b, Vector2 c) {
        a_ = a;
        b_ = b;
        c_ = c;
        return *this;
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
    Vector2 a_{}; /**< Pierwszy wierzchołek. */
    Vector2 b_{}; /**< Drugi wierzchołek. */
    Vector2 c_{}; /**< Trzeci wierzchołek. */
};

} // namespace shapes
