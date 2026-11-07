/**
 * @file Circle.hpp
 * @brief Kształt Circle - okrąg lub koło.
 */
#pragma once

#include "../Entity.hpp"

namespace shapes {

/** @brief Okrąg lub koło; pozycja obiektu oznacza środek. */
class Circle : public Entity {
  public:
    /**
     * @brief Ustawia promień.
     * @param radius Promień w pikselach.
     * @return Ten sam okrąg.
     */
    template <typename Self> Self& Radius(this Self& self, float radius) {
        self.radius_ = radius;
        return self;
    }

    /**
     * @brief Wybiera własny algorytm z ośmiokrotną symetrią zamiast raylib.
     * @param value true dla algorytmu własnego, false dla raylib.
     * @return Ten sam okrąg.
     */
    template <typename Self> Self& Symmetric(this Self& self, bool value = true) {
        self.symmetric_ = value;
        return self;
    }

    /** @brief Podaje promień. */
    float Radius() const { return radius_; }

    /** @brief Sprawdza, czy okrąg rysuje własny algorytm z symetrią. */
    bool IsSymmetric() const { return symmetric_; }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        const auto local = TryToLocal(point);
        return local && ::CheckCollisionPointCircle(*local, {0.0f, 0.0f}, radius_);
    }

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Fill(filled_).Symmetric(symmetric_).Circle({0.0f, 0.0f}, radius_);
    }

  private:
    float radius_ = 10.0f;   /**< Promień w pikselach. */
    bool symmetric_ = false; /**< Czy użyć własnego algorytmu. */
};

} // namespace shapes
