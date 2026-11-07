/**
 * @file Rect.hpp
 * @brief Kształt Rect - prostokąt.
 */
#pragma once

#include "../Entity.hpp"

namespace shapes {

/** @brief Prostokąt, którego pozycja oznacza lewy górny róg. */
class Rect : public Entity {
  public:
    /**
     * @brief Ustawia szerokość i wysokość prostokąta.
     * @param size Wymiary w pikselach.
     * @return Ten sam prostokąt.
     */
    template <typename Self> Self& Size(this Self& self, Vector2 size) {
        self.size_ = size;
        return self;
    }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        const auto local = TryToLocal(point);
        return local && ::CheckCollisionPointRec(*local, {0.0f, 0.0f, size_.x, size_.y});
    }

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Fill(filled_).Rect({0.0f, 0.0f}, size_);
    }

  private:
    Vector2 size_{10, 10}; /**< Szerokość i wysokość w pikselach. */
};

} // namespace shapes
