/**
 * @file Rect.hpp
 * @brief Kształt Rect - prostokąt.
 */
#pragma once

#include <raycade/entity/Entity.hpp>

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

    /** @brief Podaje szerokość i wysokość. */
    Vector2 Size() const { return size_; }

    /**
     * @brief Rysuje kontur własnym algorytmem przyrostowym zamiast raylib.
     * @details Nie zmienia wypełnienia (Fill()).
     * @param value true dla algorytmu własnego, false dla raylib.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Incremental(this Self& self, bool value = true) {
        self.incremental_ = value;
        return self;
    }

    /** @brief Sprawdza, czy kontur rysuje algorytm przyrostowy. */
    bool IsIncremental() const { return incremental_; }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override;

  protected:
    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    Vector2 size_{10, 10};     /**< Szerokość i wysokość w pikselach. */
    bool incremental_ = false; /**< Czy kontur rysuje algorytm przyrostowy. */
};

} // namespace shapes
