/**
 * @file Surface.hpp
 * @brief Kształt Surface - dzieci rysowane do tekstury GPU (render do tekstury).
 */
#pragma once

#include <raycade/entity/Entity.hpp>
#include <raycade/render/RenderTarget.hpp>

namespace shapes {

/**
 * @brief Powierzchnia, której dzieci rysują się do własnej tekstury GPU.
 * @details Dzieci rysują się w układzie powierzchni ((0, 0) to jej lewy górny róg) do
 * tekstury o rozmiarze Size(), a potem tekstura jest rysowana w miejscu powierzchni - razem
 * z jej obrotem, skalą, rodzicami i kamerą sceny. Co wystaje poza Size(), jest ucinane.
 *
 * Do czego się przydaje:
 * - przycinanie (np. przewijany panel, okno w oknie),
 * - półprzezroczystość całej grupy naraz (Paint() z alfą - bez prześwitywania nakładających się
 *   dzieci przez siebie),
 * - jedno przekształcenie całego rysunku (np. obrót gotowego obrazka),
 * - efekty na całym rysunku dzieci: Shader() i Blend() działają na gotowej teksturze.
 *
 * Trafienia myszą w dzieci działają jak zwykle, ale tylko wewnątrz powierzchni.
 * @code{.cpp}
 * scene.Spawn<shapes::Surface>()
 *     .Size({300, 200})
 *     .Background(Fade(BLACK, 0.2f))
 *     .Paint(Fade(WHITE, 0.7f)) // kolor mnożący teksturę
 *     .Position({50, 50})
 *     .Children([](shapes::Surface& panel) {
 *         panel.Spawn<shapes::Circle>().Position({290, 100}).Radius(40).Fill(); // ucięte
 *     });
 * @endcode
 */
class Surface : public Entity {
  public:
    /**
     * @brief Pusta powierzchnia z kolorem mnożącym WHITE (tekstura bez zmian).
     * @details Paint() działa tu jak w shapes::Sprite i shapes::Canvas: mnoży piksele gotowej
     * tekstury, np. `Paint(Fade(WHITE, 0.5f))` daje półprzezroczystą grupę.
     */
    Surface() { Paint(WHITE); }

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
     * @brief Wybiera filtr tekstury przy obrocie i skalowaniu powierzchni.
     * @param value false - ostre piksele (domyślnie), true - wygładzanie.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Smooth(this Self& self, bool value = true) {
        self.smooth_ = value;
        if (self.target_.IsValid()) self.target_.Smooth(value);
        return self;
    }

    /** @brief Podaje kolor tła tekstury. */
    Color Background() const { return background_; }

    /** @brief Sprawdza, czy tekstura jest wygładzana. */
    bool IsSmooth() const { return smooth_; }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override;

    /** @brief Udostępnia teksturę (np. do własnego rysowania jej kopii). */
    const RenderTarget& Target() const { return target_; }

  protected:
    /** @brief Powierzchnia nie ma własnego kształtu poza teksturą. */
    void Draw(PrimitiveRenderer&) const override {}

    /**
     * @details Najpierw dzieci do tekstury (w czystym układzie powierzchni), potem tekstura
     * w układzie powierzchni - z jej przekształceniem, rodzicami i kamerą.
     */
    void RenderContent(PrimitiveRenderer& renderer) const override;

    /** @brief Dzieci są trafiane tylko wewnątrz powierzchni (poza nią są ucięte). */
    bool ChildrenHittable(Vector2 point) const override { return Contains(point); }

  private:
    Vector2 size_{0, 0};          /**< Rozmiar powierzchni. */
    Color background_ = BLANK;    /**< Tło tekstury. */
    bool smooth_ = false;         /**< Filtr tekstury. */
    mutable RenderTarget target_; /**< Tekstura tworzona przy pierwszym rysowaniu. */
};

} // namespace shapes
