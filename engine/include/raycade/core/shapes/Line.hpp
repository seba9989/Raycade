/**
 * @file Line.hpp
 * @brief Kształt Line - pojedynczy odcinek.
 */
#pragma once

#include "../Entity.hpp"

namespace shapes {

/** @brief Odcinek, którego końce są określone względem pozycji obiektu. */
class Line : public Entity {
  public:
    /**
     * @brief Ustawia początek odcinka.
     * @param position Pozycja lokalna.
     * @return Ten sam odcinek.
     */
    template <typename Self> Self& From(this Self& self, Vector2 position) {
        self.from_ = position;
        return self;
    }

    /**
     * @brief Ustawia koniec odcinka.
     * @param position Pozycja lokalna.
     * @return Ten sam odcinek.
     */
    template <typename Self> Self& To(this Self& self, Vector2 position) {
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

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        return ::CheckCollisionPointLine(point, World(from_), World(to_), kHitDistance);
    }

    /** @brief Tolerancja trafienia kursorem w odcinek, w pikselach. */
    static constexpr int kHitDistance = 4;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Incremental(incremental_).Line(from_, to_);
    }

  private:
    Vector2 from_{};           /**< Początek względem pozycji obiektu. */
    Vector2 to_{};             /**< Koniec względem pozycji obiektu. */
    bool incremental_ = false; /**< Czy użyć algorytmu przyrostowego. */
};

} // namespace shapes
