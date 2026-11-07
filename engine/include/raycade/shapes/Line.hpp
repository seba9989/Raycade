/**
 * @file Line.hpp
 * @brief Kształt Line - pojedynczy odcinek.
 */
#pragma once

#include <raycade/entity/Entity.hpp>
#include <raycade/shapes/HitDistance.hpp>

namespace shapes {

/** @brief Odcinek, którego końce są określone względem pozycji obiektu. */
class Line : public Entity {
  public:
    /**
     * @brief Ustawia początek odcinka.
     * @param position Pozycja lokalna.
     * @return Ten sam odcinek.
     */
    template <typename Self> Self& From(this Self& self, Point2D position) {
        self.from_ = position;
        return self;
    }

    /**
     * @brief Ustawia koniec odcinka.
     * @param position Pozycja lokalna.
     * @return Ten sam odcinek.
     */
    template <typename Self> Self& To(this Self& self, Point2D position) {
        self.to_ = position;
        return self;
    }

    /**
     * @brief Wybiera własny algorytm przyrostowy zamiast raylib.
     * @param value Czy włączyć algorytm własny.
     * @return Ten sam odcinek.
     */
    template <typename Self> Self& Incremental(this Self& self, bool value = true) {
        self.incremental_ = value;
        return self;
    }

    /** @brief Podaje początek odcinka (względem pozycji obiektu). */
    Point2D From() const { return from_; }

    /** @brief Podaje koniec odcinka (względem pozycji obiektu). */
    Point2D To() const { return to_; }

    /** @brief Sprawdza, czy odcinek rysuje algorytm przyrostowy. */
    bool IsIncremental() const { return incremental_; }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        return ::CheckCollisionPointLine(point, World(from_), World(to_), kHitDistance);
    }

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Incremental(incremental_).Line(from_, to_);
    }

  private:
    Point2D from_;             /**< Początek względem pozycji obiektu. */
    Point2D to_;               /**< Koniec względem pozycji obiektu. */
    bool incremental_ = false; /**< Czy użyć algorytmu przyrostowego. */
};

} // namespace shapes
