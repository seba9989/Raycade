/**
 * @file Lines.hpp
 * @brief Kształt Lines - zbiór niezależnych odcinków (wersja tablicowa).
 */
#pragma once

#include <utility>
#include <vector>

#include "../Entity.hpp"
#include "Line.hpp"

namespace shapes {

/** @brief Zbiór niezależnych odcinków tworzonych z kolejnych par wierzchołków. */
class Lines : public Entity {
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

    /**
     * @brief Wybiera algorytm przyrostowy dla odcinków.
     * @param value Czy włączyć algorytm własny.
     * @return Ten sam zbiór.
     */
    template <typename Self> Self& Incremental(this Self& self, bool value = true) {
        self.incremental_ = value;
        return self;
    }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        for (std::size_t i = 0; i + 1 < vertices_.size(); i += 2)
            if (::CheckCollisionPointLine(point, World(vertices_[i]), World(vertices_[i + 1]),
                                          Line::kHitDistance))
                return true;
        return false;
    }

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Incremental(incremental_).Lines(vertices_);
    }

  private:
    std::vector<Point2D> vertices_; /**< Pary końców odcinków. */
    bool incremental_ = false;      /**< Czy użyć algorytmu przyrostowego. */
};

} // namespace shapes
