/**
 * @file Shape.hpp
 * @brief Baza figur geometrycznych z przestrzeni nazw @ref node::shapes.
 */
#pragma once

#include <vector>

#include <raycade/entity/Entity.hpp>
#include <raycade/math/geometry/Convex.hpp>
#include <raycade/render/primitives/PrimitiveRenderer.hpp>

namespace node::shapes {

/**
 * @brief Wspólna baza figur geometrycznych: wypełnienie i algorytm rasteryzacji.
 * @details Po Shape dziedziczą tylko figury opisane geometrią (punkty, odcinki, łamane,
 * wielokąty, okręgi i elipsy). Obiekty z obrazem albo tekstem (@ref node::visual) i kontenery
 * (Group, node::visual::RenderGroup) dziedziczą wprost po Entity, więc nie mają Filled() ani
 * Raster().
 *
 * Filled() działa w figurach z wnętrzem: Rect, Triangle, Polygon, Circle i Ellipse. Point, Points,
 * Line, Lines i Polyline je ignorują.
 *
 * Raster(Rasterizer::Custom) włącza algorytm z instrukcji laboratoryjnej zamiast raylib,
 * a Raster(Rasterizer::Midpoint) - wariant okręgu i elipsy bez trygonometrii:
 * | Figura                         | Rasterizer::Custom                      | Rasterizer::Midpoint
 * |
 * |--------------------------------|-----------------------------------------|--------------------------|
 * | Line, Lines, Polyline          | przyrostowy algorytm odcinka            | jak Custom | |
 * kontur Rect, Triangle, Polygon | przyrostowy algorytm odcinka            | jak Custom | | Circle
 * | parametryczny, symetria ośmiokrotna     | punktu środkowego, 8x    | | Ellipse | parametryczny,
 * symetria czterokrotna    | punktu środkowego, 4x    | | Point, Points                  | brak
 * różnicy - piksel jest zawsze ten sam | jak Custom             | Wypełnienia Rect, Triangle i
 * Polygon nie zależą od wyboru algorytmu. Szczegóły algorytmów są przy PrimitiveRenderer::Raster().
 * @code{.cpp}
 * scene.Spawn<node::shapes::Circle>().Radius(40).Raster(Rasterizer::Custom).Paint(RED);
 * @endcode
 */
class Shape : public Entity {
  public:
    /**
     * @brief Włącza lub wyłącza wypełnienie figury.
     * @param value Czy figura ma być wypełniona.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Filled(this Self& self, bool value = true) {
        self.filled_ = value;
        return self;
    }

    /** @brief Sprawdza, czy figura jest wypełniona (Filled()). */
    bool IsFilled() const { return filled_; }

    /**
     * @brief Wybiera algorytm rasteryzacji figury.
     * @param raster Rasterizer::Raylib (domyślnie), Rasterizer::Custom (algorytm z instrukcji)
     * albo Rasterizer::Midpoint (okrąg i elipsa algorytmem punktu środkowego).
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Raster(this Self& self, Rasterizer raster) {
        self.raster_ = raster;
        return self;
    }

    /** @brief Podaje wybrany algorytm rasteryzacji. */
    Rasterizer Raster() const { return raster_; }

    /**
     * @brief Dopisuje figurę jako części wypukłe w układzie lokalnym (do kolizji).
     * @details Z tej geometrii korzysta Collider - kolizja ma zawsze ten sam kształt co figura.
     * | Figura                | Części                                              |
     * |-----------------------|-----------------------------------------------------|
     * | Point, Points         | punkty                                              |
     * | Line, Lines, Polyline | odcinki (także zamknięta łamana - tylko obrys)      |
     * | Rect, Triangle        | jeden wielokąt wypukły                              |
     * | Polygon               | jeden wielokąt albo trójkąty (wielokąt wklęsły)     |
     * | Circle, Ellipse       | elipsa                                              |
     * Wielokąt wiązany (IsSimple() == false) nie ma części, tak jak nie jest rysowany.
     * @param parts Lista, do której figura dopisuje swoje części.
     */
    virtual void ConvexParts([[maybe_unused]] std::vector<ConvexPart>& parts) const {}

  protected:
    /**
     * @brief Ustawia na rendererze pędzel figury: kolor, wypełnienie i algorytm.
     * @param renderer Renderer, którym figura się rysuje.
     * @return Ten sam renderer - do wywołania rysującego w tym samym łańcuchu.
     */
    PrimitiveRenderer& Brush(PrimitiveRenderer& renderer) const {
        return renderer.Paint(color_).Filled(filled_).Raster(raster_);
    }

    bool filled_ = false;                    /**< Czy figura jest wypełniona. */
    Rasterizer raster_ = Rasterizer::Raylib; /**< Algorytm rasteryzacji. */
};

} // namespace node::shapes
