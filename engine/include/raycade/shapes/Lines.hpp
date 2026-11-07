/**
 * @file Lines.hpp
 * @brief Kształt Lines - zbiór niezależnych odcinków (wersja tablicowa).
 */
#pragma once

#include <utility>
#include <vector>

#include <raycade/entity/Entity.hpp>
#include <raycade/shapes/HitDistance.hpp>

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

    /** @brief Podaje pary końców odcinków. */
    const std::vector<Point2D>& Vertices() const { return vertices_; }

    /** @brief Sprawdza, czy odcinki rysuje algorytm przyrostowy. */
    bool IsIncremental() const { return incremental_; }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    std::vector<Point2D> vertices_; /**< Pary końców odcinków. */
    bool incremental_ = false;      /**< Czy użyć algorytmu przyrostowego. */
};

} // namespace shapes
