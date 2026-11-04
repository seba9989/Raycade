/**
 * @file RoundShapes.cpp
 * @brief Algorytmy okręgu i elipsy z wykorzystaniem symetrii.
 * @details Okrąg wykorzystuje osiem odbić punktu obliczonego w wycinku pi/4, a elipsa
 * cztery odbicia punktu obliczonego w ćwiartce.
 */

#include "../PrimitiveRenderer.hpp"

#include <algorithm>
#include <cmath>

namespace {

/** @brief Zaokrągla współrzędną do najbliższego piksela. */
int Round(float value) {
    return static_cast<int>(std::lround(value));
}

} // namespace

PrimitiveRenderer& PrimitiveRenderer::Circle(Vector2 center, float radius) {
    if (symmetric_) {
        CircleSymmetric(center, radius);
    } else if (target_) {
        if (filled_)
            ::ImageDrawCircleV(&target_->Edit(), center, Round(radius), color_);
        else
            ::ImageDrawCircleLinesV(&target_->Edit(), center, Round(radius), color_);
    } else {
        if (filled_)
            ::DrawCircleV(center, radius, color_);
        else
            ::DrawCircleLinesV(center, radius, color_);
    }
    return *this;
}

/**
 * @details Okrąg: x = x0 + R cos(a), y = y0 + R sin(a), z ośmiokrotną symetrią. Liczone jest
 * tylko a z przedziału <0, pi/4>, a pozostałe 7 punktów powstaje przez zamianę x i y oraz
 * zmianę znaków. Krok kąta ~1/R sprawia, że sąsiednie punkty dzieli najwyżej 1 piksel.
 * Wypełnienie: zamiast 8 punktów rysowane są 4 poziome odcinki między punktami symetrycznymi.
 */
void PrimitiveRenderer::CircleSymmetric(Vector2 center, float radius) {
    const int cx = Round(center.x);
    const int cy = Round(center.y);
    const int steps = std::max(1, static_cast<int>(std::ceil(radius * PI / 4.0f)));

    for (int i = 0; i <= steps; ++i) {
        const float angle = (PI / 4.0f) * static_cast<float>(i) / static_cast<float>(steps);
        const int x = Round(radius * std::cos(angle));
        const int y = Round(radius * std::sin(angle));

        if (filled_) {
            Span(cy + y, cx - x, cx + x);
            Span(cy - y, cx - x, cx + x);
            Span(cy + x, cx - y, cx + y);
            Span(cy - x, cx - y, cx + y);
        } else {
            Plot(cx + x, cy + y);
            Plot(cx - x, cy + y);
            Plot(cx + x, cy - y);
            Plot(cx - x, cy - y);
            Plot(cx + y, cy + x);
            Plot(cx - y, cy + x);
            Plot(cx + y, cy - x);
            Plot(cx - y, cy - x);
        }
    }
}

PrimitiveRenderer& PrimitiveRenderer::Ellipse(Vector2 center, Vector2 radius) {
    // raylib nie ma funkcji rysującej elipsę na Image, więc na bitmapie zawsze własny algorytm
    if (symmetric_ || target_) {
        EllipseSymmetric(center, radius);
    } else {
        if (filled_)
            ::DrawEllipse(Round(center.x), Round(center.y), radius.x, radius.y, color_);
        else
            ::DrawEllipseLines(Round(center.x), Round(center.y), radius.x, radius.y, color_);
    }
    return *this;
}

/**
 * @details Elipsa: x = x0 + RX cos(a), y = y0 + RY sin(a), z czterokrotną symetrią -
 * liczone jest a z przedziału <0, pi/2>, resztę dają zmiany znaków (symetria ośmiokrotna
 * działa tylko dla okręgu). Krok kąta ~1/max(RX, RY), żeby punkty stykały się także wzdłuż
 * dłuższej osi.
 */
void PrimitiveRenderer::EllipseSymmetric(Vector2 center, Vector2 radius) {
    const int cx = Round(center.x);
    const int cy = Round(center.y);
    const float longer = std::max(radius.x, radius.y);
    const int steps = std::max(1, static_cast<int>(std::ceil(longer * PI / 2.0f)));

    for (int i = 0; i <= steps; ++i) {
        const float angle = (PI / 2.0f) * static_cast<float>(i) / static_cast<float>(steps);
        const int x = Round(radius.x * std::cos(angle));
        const int y = Round(radius.y * std::sin(angle));

        if (filled_) {
            Span(cy + y, cx - x, cx + x);
            Span(cy - y, cx - x, cx + x);
        } else {
            Plot(cx + x, cy + y);
            Plot(cx - x, cy + y);
            Plot(cx + x, cy - y);
            Plot(cx - x, cy - y);
        }
    }
}
