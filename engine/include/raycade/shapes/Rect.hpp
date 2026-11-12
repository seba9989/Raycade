/**
 * @file Rect.hpp
 * @brief Kształt Rect - prostokąt.
 */
#pragma once

#include <raycade/shapes/Shape.hpp>

namespace shapes {

/** @brief Prostokąt, którego pozycja oznacza lewy górny róg. */
class Rect : public Shape {
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

    /** @brief Podaje szerokość i wysokość. */
    Vector2 Size() const { return size_; }

    /** @copydoc Entity::Contains */
    bool Contains(Vector2 point) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    Vector2 size_{10, 10}; /**< Szerokość i wysokość w pikselach. */
};

} // namespace shapes
