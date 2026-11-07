/**
 * @file RoundShapes.cpp
 * @brief Algorytmy okręgu i elipsy z wykorzystaniem symetrii.
 * @details Okrąg: algorytm punktu środkowego (Bresenhama) na liczbach całkowitych dla oktantu
 * od (0, r) do x = y i osiem odbić każdego punktu. Elipsa: ten sam algorytm w dwóch obszarach
 * ćwiartki (nachylenie powyżej i poniżej -1) i cztery odbicia każdego punktu.
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

/**
 * @brief Algorytm punktu środkowego dla ćwiartki elipsy od (0, b) do (a, 0).
 * @details Wywołuje plot(x, y) dla każdego punktu ćwiartki. Wymaga a > 0 i b > 0.
 */
template <typename PlotFn> void EllipseQuadrant(long long a, long long b, PlotFn plot) {
    const long long a2 = a * a;
    const long long b2 = b * b;
    // Algorytm punktu środkowego dla ćwiartki od (0, b) do (a, 0), F(x, y) = b²x² + a²y² - a²b².
    // Zmienna decyzyjna jest przemnożona przez 4, żeby połówki punktu środkowego dały liczby
    // całkowite. Obszar 1 (nachylenie > -1): x rośnie o 1, a y maleje, gdy punkt środkowy
    // (x + 1, y - 1/2) leży poza elipsą.
    long long x = 0;
    long long y = b;
    long long d = 4 * b2 - 4 * a2 * b + a2; // 4 F(1, b - 1/2)
    while (b2 * x <= a2 * y) {
        plot(x, y);
        if (d < 0) {
            d += 4 * b2 * (2 * x + 3); // punkt E
        } else {
            d += 4 * b2 * (2 * x + 3) - 8 * a2 * (y - 1); // punkt SE
            --y;
        }
        ++x;
    }
    // Obszar 2 (nachylenie < -1): y maleje o 1, a x rośnie, gdy punkt środkowy
    // (x + 1/2, y - 1) leży wewnątrz elipsy.
    d = b2 * (2 * x + 1) * (2 * x + 1) + 4 * a2 * (y - 1) * (y - 1) - 4 * a2 * b2;
    while (y >= 0) {
        plot(x, y);
        if (d > 0) {
            d += 4 * a2 * (3 - 2 * y); // punkt S
        } else {
            d += 8 * b2 * (x + 1) + 4 * a2 * (3 - 2 * y); // punkt SE
            ++x;
        }
        --y;
    }
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
    // te same zaokrąglenia co w CircleSymmetric(), więc Ellipse({r, r}) daje okrąg Circle(r)
    const long long a = Round(std::fabs(radius.x));
    const long long b = Round(std::fabs(radius.y));

    RowExtents rows(cy - static_cast<int>(b), cy + static_cast<int>(b));
    std::vector<std::pair<int, int>> pixels;
    const auto reflect = [&](long long px, long long py) {
        const int x = static_cast<int>(px);
        const int y = static_cast<int>(py);
        if (filled_) {
            rows.Add(cy + y, cx - x, cx + x);
            rows.Add(cy - y, cx - x, cx + x);
        } else {
            pixels.insert(pixels.end(),
                          {{cx + x, cy + y}, {cx - x, cy + y}, {cx + x, cy - y}, {cx - x, cy - y}});
        }
    };

    if (a == 0 || b == 0) {
        // elipsa zdegenerowana do odcinka - w EllipseQuadrant() warunek obszaru 1 nigdy by
        // się nie zmienił
        for (long long t = 0; t <= std::max(a, b); ++t)
            reflect(a == 0 ? 0 : t, b == 0 ? 0 : t);
    } else {
        EllipseQuadrant(a, b, reflect);
    }

    if (filled_) {
        rows.ForEach([this](int row, int fromX, int toX) { Span(row, fromX, toX); });
    } else {
        Deduplicate(pixels);
        for (const auto& [px, py] : pixels)
            Plot(px, py);
    }
}
