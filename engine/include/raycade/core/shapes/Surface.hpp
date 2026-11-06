/**
 * @file Surface.hpp
 * @brief Kształt Surface - dzieci rysowane do tekstury GPU (render do tekstury).
 */
#pragma once

#include "../../render/RenderTarget.hpp"
#include "../Entity.hpp"

namespace shapes {

/**
 * @brief Powierzchnia, której dzieci rysują się do własnej tekstury GPU.
 * @details Dzieci rysują się w układzie powierzchni ((0, 0) to jej lewy górny róg) do
 * tekstury o rozmiarze Size(), a potem tekstura jest rysowana w miejscu powierzchni - razem
 * z jej obrotem, skalą, rodzicami i kamerą sceny. Co wystaje poza Size(), jest ucinane.
 *
 * Do czego się przydaje:
 * - przycinanie (np. przewijany panel, okno w oknie),
 * - półprzezroczystość całej grupy naraz (Tint() z alfą - bez prześwitywania nakładających się
 *   dzieci przez siebie),
 * - jedno przekształcenie całego rysunku (np. obrót gotowego obrazka),
 * - podstawa efektów na całym obrazie (shadery - w przyszłości).
 *
 * Trafienia myszą w dzieci działają jak zwykle, ale tylko wewnątrz powierzchni.
 * @code{.cpp}
 * scene.Spawn<shapes::Surface>()
 *     .Size({300, 200})
 *     .Background(Fade(BLACK, 0.2f))
 *     .Tint(Fade(WHITE, 0.7f))
 *     .Position({50, 50})
 *     .Children([](shapes::Surface& panel) {
 *         panel.Spawn<shapes::Circle>().Position({290, 100}).Radius(40).Fill(); // ucięte
 *     });
 * @endcode
 */
class Surface : public Entity {
  public:
    /**
     * @brief Ustawia rozmiar powierzchni (i jej tekstury).
     * @details Tekstura powstaje przy pierwszym rysowaniu, więc rozmiar można ustawić
     * jeszcze przed otwarciem okna.
     * @param size Szerokość i wysokość w pikselach.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Size(this Self& self, Vector2 size) {
        self.size_ = size;
        self.target_ = RenderTarget{};
        return self;
    }

    /** @brief Zwraca rozmiar powierzchni. */
    Vector2 Size() const { return size_; }

    /**
     * @brief Ustawia kolor tła tekstury czyszczonej co klatkę.
     * @param color Kolor (domyślnie BLANK - przezroczyste).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Background(this Self& self, Color color) {
        self.background_ = color;
        return self;
    }

    /**
     * @brief Ustawia kolor mnożący przy rysowaniu tekstury (np. Fade(WHITE, 0.5f)).
     * @param tint Kolor (domyślnie WHITE - bez zmian).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Tint(this Self& self, Color tint) {
        self.tint_ = tint;
        return self;
    }

    /**
     * @brief Wybiera filtr tekstury przy obrocie i skalowaniu powierzchni.
     * @param value false - ostre piksele (domyślnie), true - wygładzanie.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Smooth(this Self& self, bool value = true) {
        self.smooth_ = value;
        if (self.target_.Valid()) self.target_.Smooth(value);
        return self;
    }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        const auto local = TryToLocal(point);
        return local && ::CheckCollisionPointRec(*local, {0, 0, size_.x, size_.y});
    }

    /** @brief Udostępnia teksturę (np. do własnego rysowania jej kopii). */
    const RenderTarget& Target() const { return target_; }

  protected:
    /** @brief Powierzchnia nie ma własnego kształtu poza teksturą. */
    void Draw(PrimitiveRenderer&) const override {}

    /**
     * @details Najpierw dzieci do tekstury (w czystym układzie powierzchni), potem tekstura
     * w układzie powierzchni - z jej przekształceniem, rodzicami i kamerą.
     */
    void RenderContent(PrimitiveRenderer& renderer) const override {
        if (size_.x < 1 || size_.y < 1) return;
        if (!target_.Valid()) {
            target_ = RenderTarget{static_cast<int>(size_.x), static_cast<int>(size_.y)};
            target_.Smooth(smooth_);
        }
        target_.Begin();
        target_.Clear(background_);
        RenderChildren(renderer);
        target_.End();
        target_.Draw(Vector2{0, 0}, tint_);
    }

    /** @brief Dzieci są trafiane tylko wewnątrz powierzchni (poza nią są ucięte). */
    bool ChildrenHittable(Vector2 point) const override { return Contains(point); }

  private:
    Vector2 size_{0, 0};          /**< Rozmiar powierzchni. */
    Color background_ = BLANK;    /**< Tło tekstury. */
    Color tint_ = WHITE;          /**< Kolor mnożący przy rysowaniu tekstury. */
    bool smooth_ = false;         /**< Filtr tekstury. */
    mutable RenderTarget target_; /**< Tekstura tworzona przy pierwszym rysowaniu. */
};

} // namespace shapes
