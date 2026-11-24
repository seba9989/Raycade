/**
 * @file RoundShapes.cpp
 * @brief Algorytmy okręgu i elipsy z wykorzystaniem symetrii.
 * @details Dwie rodziny algorytmów:
 * - parametryczne z instrukcji (Rasterizer::Custom): x = x0 + R cos(a), y = y0 + R sin(a),
 *   liczone tylko dla a z przedziału <0, pi/4> (okrąg, symetria ośmiokrotna) albo <0, pi/2>
 *   (elipsa, symetria czterokrotna); resztę krzywej dają zamiana x i y oraz zmiany znaków,
 * - punktu środkowego (Rasterizer::Midpoint): ta sama symetria, ale arytmetyka całkowita
 *   i zmienna decyzyjna zamiast funkcji trygonometrycznych.
 *
 * Odbicia (i sąsiednie kroki z tym samym zaokrąglonym punktem) trafiają w te same piksele,
 * dlatego piksele są najpierw zbierane, a dopiero potem rysowane - każdy dokładnie raz.
 * Inaczej półprzezroczysty kolor na ekranie (DrawPixel() miesza alfę) dawałby ciemniejsze
 * pasy w wypełnieniu i ciemniejsze punkty w konturze.
 */

#include <raycade/render/primitives/PrimitiveRenderer.hpp>

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

/**
 * @brief Zbiera piksele krzywej z odbić symetrycznych i rysuje je na końcu.
 * @details Kontur: lista pikseli bez powtórzeń. Wypełnienie: dla każdego punktu (x, y)
 * poziomy odcinek między nim a jego odbiciem (-x, y) - wiersze są łączone przez RowExtents.
 * Współrzędne punktów są względem środka (cx, cy).
 */
class SymmetricRaster {
  public:
    /** @brief Krzywa o środku (cx, cy) mieszcząca się w wierszach cy - halfHeight .. cy +
     * halfHeight. */
    SymmetricRaster(int cx, int cy, int halfHeight, bool filled)
        : cx_(cx), cy_(cy), filled_(filled), rows_(cy - halfHeight, cy + halfHeight) {}

    /** @brief Symetria czterokrotna: (x, y), (-x, y), (x, -y), (-x, -y). */
    void Mirror4(int x, int y) {
        if (filled_) {
            rows_.Add(cy_ + y, cx_ - x, cx_ + x);
            rows_.Add(cy_ - y, cx_ - x, cx_ + x);
        } else {
            pixels_.insert(
                pixels_.end(),
                {{cx_ + x, cy_ + y}, {cx_ - x, cy_ + y}, {cx_ + x, cy_ - y}, {cx_ - x, cy_ - y}}
            );
        }
    }

    /** @brief Symetria ośmiokrotna: cztery odbicia (x, y) i cztery odbicia (y, x). */
    void Mirror8(int x, int y) {
        Mirror4(x, y);
        Mirror4(y, x);
    }

    /** @brief Rysuje zebrane piksele (plot) albo wiersze wypełnienia (span), każdy raz. */
    template <typename PlotFn, typename SpanFn> void Flush(PlotFn plot, SpanFn span) {
        if (filled_) {
            rows_.ForEach(span);
            return;
        }
        std::sort(pixels_.begin(), pixels_.end());
        pixels_.erase(std::unique(pixels_.begin(), pixels_.end()), pixels_.end());
        for (const auto& [px, py] : pixels_)
            plot(px, py);
    }

  private:
    int cx_;
    int cy_;
    bool filled_;
    RowExtents rows_;
    std::vector<std::pair<int, int>> pixels_;
};

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
    if (raster_ == Rasterizer::Custom) {
        CircleParametric(center, radius);
    } else if (raster_ == Rasterizer::Midpoint) {
        CircleMidpoint(center, radius);
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
 * @details Algorytm z instrukcji: x = R cos(a), y = R sin(a) dla a z przedziału <0, pi/4>,
 * a każdy punkt daje osiem pikseli (x0 ± x, y0 ± y) i (x0 ± y, y0 ± x) - zamiana x i y oraz
 * zmiany znaków. Krok kąta 1/R odpowiada łukowi długości 1 piksela, więc kolejne punkty
 * różnią się najwyżej o 1 w każdej osi i kontur nie ma dziur. Ostatni krok trafia dokładnie
 * w a = pi/4, żeby oktanty się domknęły.
 *
 * Wypełnienie: zamiast pikseli zbierane są poziome odcinki między punktami symetrycznymi
 * względem osi pionowej.
 */
void PrimitiveRenderer::CircleParametric(Vector2 center, float radius) {
    const int cx = Round(center.x);
    const int cy = Round(center.y);
    const int r = Round(std::fabs(radius));

    SymmetricRaster raster(cx, cy, r, filled_);
    if (r == 0) {
        raster.Mirror4(0, 0);
    } else {
        constexpr float kOctant = PI / 4.0f;
        const int steps = static_cast<int>(std::ceil(kOctant * static_cast<float>(r)));
        for (int i = 0; i <= steps; ++i) {
            const float alpha = kOctant * static_cast<float>(i) / static_cast<float>(steps);
            const int x = Round(static_cast<float>(r) * std::cos(alpha));
            const int y = Round(static_cast<float>(r) * std::sin(alpha));
            raster.Mirror8(x, y);
        }
    }
    raster.Flush(
        [this](int x, int y) { Plot(x, y); },
        [this](int row, int fromX, int toX) { Span(row, fromX, toX); }
    );
}

/**
 * @details Oktant od (0, r) do x = y: x rośnie o 1, a y maleje o 1, gdy punkt środkowy
 * (x + 1, y - 1/2) leży poza okręgiem. Zmienna decyzyjna d = F(x + 1, y - 1/2), gdzie
 * F(x, y) = x² + y² - r², przemnożona przez 4 i zaokrąglona do liczby całkowitej - dlatego
 * start 1 - r zamiast 5/4 - r. Każdy punkt daje osiem pikseli, jak w CircleParametric().
 */
void PrimitiveRenderer::CircleMidpoint(Vector2 center, float radius) {
    const int cx = Round(center.x);
    const int cy = Round(center.y);
    const int r = Round(std::fabs(radius));

    SymmetricRaster raster(cx, cy, r, filled_);
    int x = 0;
    int y = r;
    int d = 1 - r;
    while (x <= y) {
        raster.Mirror8(x, y);
        if (d < 0) {
            d += 2 * x + 3; // punkt E: środek wewnątrz okręgu
        } else {
            d += 2 * (x - y) + 5; // punkt SE: środek na zewnątrz
            --y;
        }
        ++x;
    }
    raster.Flush(
        [this](int px, int py) { Plot(px, py); },
        [this](int row, int fromX, int toX) { Span(row, fromX, toX); }
    );
}

PrimitiveRenderer& PrimitiveRenderer::Ellipse(Point2D center, Vector2 radius) {
    if (raster_ == Rasterizer::Custom) {
        EllipseParametric(center, radius);
    } else if (raster_ == Rasterizer::Midpoint || target_) {
        // raylib nie ma funkcji rysującej elipsę na Image - na bitmapie zastępuje go algorytm
        // punktu środkowego, najbliższy wynikiem funkcji raylib
        EllipseMidpoint(center, radius);
    } else {
        if (filled_)
            ::DrawEllipse(Round(center.X()), Round(center.Y()), radius.x, radius.y, color_);
        else
            ::DrawEllipseLines(Round(center.X()), Round(center.Y()), radius.x, radius.y, color_);
    }
    return *this;
}

/**
 * @details Algorytm z instrukcji: x = RX cos(a), y = RY sin(a) dla a z przedziału <0, pi/2>,
 * a każdy punkt daje cztery piksele (x0 ± x, y0 ± y). Symetria ośmiokrotna działa tylko dla
 * okręgu - elipsa nie jest symetryczna względem prostej y = x. Krok kąta 1/max(RX, RY), żeby
 * kolejne punkty stykały się także wzdłuż dłuższej osi. Dla RX = RY powstaje okrąg, ale
 * piksele mogą się nieznacznie różnić od CircleParametric() - tam kąt jest próbkowany tylko
 * do pi/4, a druga połowa ćwiartki powstaje z zamiany x i y.
 */
void PrimitiveRenderer::EllipseParametric(Vector2 center, Vector2 radius) {
    const int cx = Round(center.x);
    const int cy = Round(center.y);
    const int a = Round(std::fabs(radius.x));
    const int b = Round(std::fabs(radius.y));

    SymmetricRaster raster(cx, cy, b, filled_);
    const int longer = std::max(a, b);
    if (longer == 0) {
        raster.Mirror4(0, 0);
    } else {
        constexpr float kQuadrant = PI / 2.0f;
        const int steps = static_cast<int>(std::ceil(kQuadrant * static_cast<float>(longer)));
        for (int i = 0; i <= steps; ++i) {
            const float alpha = kQuadrant * static_cast<float>(i) / static_cast<float>(steps);
            const int x = Round(static_cast<float>(a) * std::cos(alpha));
            const int y = Round(static_cast<float>(b) * std::sin(alpha));
            raster.Mirror4(x, y);
        }
    }
    raster.Flush(
        [this](int x, int y) { Plot(x, y); },
        [this](int row, int fromX, int toX) { Span(row, fromX, toX); }
    );
}

/**
 * @details Ćwiartka jest liczona w dwóch obszarach - tam, gdzie krzywa jest bardziej
 * pozioma (krok w x), i tam, gdzie bardziej pionowa (krok w y); każdy punkt daje cztery
 * piksele. Dla równych promieni wynik jest taki sam jak CircleMidpoint().
 */
void PrimitiveRenderer::EllipseMidpoint(Vector2 center, Vector2 radius) {
    const int cx = Round(center.x);
    const int cy = Round(center.y);
    const long long a = Round(std::fabs(radius.x));
    const long long b = Round(std::fabs(radius.y));

    SymmetricRaster raster(cx, cy, static_cast<int>(b), filled_);
    const auto mirror = [&raster](long long x, long long y) {
        raster.Mirror4(static_cast<int>(x), static_cast<int>(y));
    };
    if (a == 0 || b == 0) {
        // elipsa zdegenerowana do odcinka - w EllipseQuadrant() warunek obszaru 1 nigdy by
        // się nie zmienił
        for (long long t = 0; t <= std::max(a, b); ++t)
            mirror(a == 0 ? 0 : t, b == 0 ? 0 : t);
    } else {
        EllipseQuadrant(a, b, mirror);
    }
    raster.Flush(
        [this](int px, int py) { Plot(px, py); },
        [this](int row, int fromX, int toX) { Span(row, fromX, toX); }
    );
}
