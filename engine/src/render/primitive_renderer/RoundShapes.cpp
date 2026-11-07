/**
 * @file RoundShapes.cpp
 * @brief Algorytmy okręgu i elipsy z wykorzystaniem symetrii.
 * @details Okrąg: algorytm punktu środkowego (Bresenhama) na liczbach całkowitych dla oktantu
 * od (0, r) do x = y i osiem odbić każdego punktu. Elipsa: cztery odbicia punktu obliczonego
 * w ćwiartce.
 *
 * Odbicia (i sąsiednie kroki z tym samym zaokrąglonym punktem) trafiają w te same piksele,
 * dlatego piksele są najpierw zbierane, a dopiero potem rysowane - każdy dokładnie raz.
 * Inaczej półprzezroczysty kolor na ekranie (DrawPixel() miesza alfę) dawałby ciemniejsze
 * pasy w wypełnieniu i ciemniejsze punkty w konturze.
 */

#include <raycade/render/PrimitiveRenderer.hpp>

#include "Pixel.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <vector>

using pixel::Round;

namespace {

/**
 * @brief Zakres x wypełnienia w każdym wierszu figury.
 * @details Spany z różnych odbić tego samego wiersza nakładają się - zapamiętujemy tylko
 * najmniejsze i największe x, żeby wiersz narysować jednym spanem.
 */
class RowExtents {
  public:
    /** @brief Wiersze od top do bottom włącznie. */
    RowExtents(int top, int bottom)
        : top_(top), rows_(static_cast<std::size_t>(bottom - top + 1), {INT_MAX, INT_MIN}) {}

    /** @brief Poszerza zakres wiersza y o przedział [fromX, toX]. */
    void Add(int y, int fromX, int toX) {
        if (y < top_ || y - top_ >= static_cast<int>(rows_.size())) return;
        auto& [low, high] = rows_[static_cast<std::size_t>(y - top_)];
        low = std::min({low, fromX, toX});
        high = std::max({high, fromX, toX});
    }

    /** @brief Wywołuje span(y, fromX, toX) dla każdego niepustego wiersza. */
    template <typename SpanFn> void ForEach(SpanFn span) const {
        for (std::size_t i = 0; i < rows_.size(); ++i)
            if (rows_[i].first <= rows_[i].second)
                span(top_ + static_cast<int>(i), rows_[i].first, rows_[i].second);
    }

  private:
    int top_;
    std::vector<std::pair<int, int>> rows_;
};

/** @brief Usuwa powtórzone piksele konturu (kolejność rysowania nie ma znaczenia). */
void Deduplicate(std::vector<std::pair<int, int>>& pixels) {
    std::sort(pixels.begin(), pixels.end());
    pixels.erase(std::unique(pixels.begin(), pixels.end()), pixels.end());
}

} // namespace

PrimitiveRenderer& PrimitiveRenderer::Circle(Point2D center, float radius) {
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
    const int r = Round(std::fabs(radius));

    // Algorytm punktu środkowego dla oktantu od (0, r) do x = y: x rośnie o 1, a y maleje
    // o 1, gdy punkt środkowy (x + 1, y - 1/2) leży poza okręgiem. Zmienna decyzyjna
    // d = F(x + 1, y - 1/2), gdzie F(x, y) = x^2 + y^2 - r^2, przemnożona przez 4 i
    // zaokrąglona do liczby całkowitej - dlatego start 1 - r zamiast 5/4 - r.
    RowExtents rows(cy - r, cy + r);
    std::vector<std::pair<int, int>> pixels;
    int x = 0;
    int y = r;
    int d = 1 - r;
    while (x <= y) {
        if (filled_) {
            rows.Add(cy + y, cx - x, cx + x);
            rows.Add(cy - y, cx - x, cx + x);
            rows.Add(cy + x, cx - y, cx + y);
            rows.Add(cy - x, cx - y, cx + y);
        } else {
            pixels.insert(pixels.end(), {{cx + x, cy + y},
                                         {cx - x, cy + y},
                                         {cx + x, cy - y},
                                         {cx - x, cy - y},
                                         {cx + y, cy + x},
                                         {cx - y, cy + x},
                                         {cx + y, cy - x},
                                         {cx - y, cy - x}});
        }
        if (d < 0) {
            d += 2 * x + 3; // punkt E: środek wewnątrz okręgu
        } else {
            d += 2 * (x - y) + 5; // punkt SE: środek na zewnątrz
            --y;
        }
        ++x;
    }
    if (filled_) {
        rows.ForEach([this](int row, int fromX, int toX) { Span(row, fromX, toX); });
    } else {
        Deduplicate(pixels);
        for (const auto& [px, py] : pixels)
            Plot(px, py);
    }
}

PrimitiveRenderer& PrimitiveRenderer::Ellipse(Point2D center, Vector2 radius) {
    // raylib nie ma funkcji rysującej elipsę na Image, więc na bitmapie zawsze własny algorytm
    if (symmetric_ || target_) {
        EllipseSymmetric(center, radius);
    } else {
        if (filled_)
            ::DrawEllipse(Round(center.X()), Round(center.Y()), radius.x, radius.y, color_);
        else
            ::DrawEllipseLines(Round(center.X()), Round(center.Y()), radius.x, radius.y, color_);
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
    const int extent = std::abs(Round(radius.y));
    const float longer = std::max(radius.x, radius.y);
    const int steps = std::max(1, static_cast<int>(std::ceil(longer * PI / 2.0f)));

    RowExtents rows(cy - extent, cy + extent);
    std::vector<std::pair<int, int>> pixels;
    for (int i = 0; i <= steps; ++i) {
        const float angle = (PI / 2.0f) * static_cast<float>(i) / static_cast<float>(steps);
        const int x = Round(radius.x * std::cos(angle));
        const int y = Round(radius.y * std::sin(angle));

        if (filled_) {
            rows.Add(cy + y, cx - x, cx + x);
            rows.Add(cy - y, cx - x, cx + x);
        } else {
            pixels.insert(pixels.end(),
                          {{cx + x, cy + y}, {cx - x, cy + y}, {cx + x, cy - y}, {cx - x, cy - y}});
        }
    }
    if (filled_) {
        rows.ForEach([this](int row, int fromX, int toX) { Span(row, fromX, toX); });
    } else {
        Deduplicate(pixels);
        for (const auto& [px, py] : pixels)
            Plot(px, py);
    }
}
