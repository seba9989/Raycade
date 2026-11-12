/**
 * @file Lines.hpp
 * @brief Kształt Lines - zbiór niezależnych odcinków (wersja tablicowa).
 */
#pragma once

#include <utility>
#include <vector>

#include <raycade/shapes/Shape.hpp>

namespace shapes {

/** @brief Zbiór niezależnych odcinków tworzonych z kolejnych par wierzchołków. */
class Lines : public Shape {
  public:
    /**
     * @brief Ustawia pary końców odcinków.
     * @param vertices Wierzchołki (0,1), (2,3) itd., lokalne względem pozycji obiektu.
     * @return Ten sam zbiór.
     */
    template <typename Self> Self& Vertices(this Self& self, std::vector<Point2D> vertices) {
        self.vertices_ = std::move(vertices);
        return self;
    }

    /** @brief Podaje pary końców odcinków. */
    const std::vector<Point2D>& Vertices() const { return vertices_; }

    /** @copydoc Entity::Contains */
    bool Contains(Vector2 point) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    std::vector<Point2D> vertices_; /**< Pary końców odcinków. */
};

} // namespace shapes
