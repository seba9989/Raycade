/**
 * @file Polygon.cpp
 * @brief Rysowanie wielokątów i wypełnianie poziomymi wierszami (scanline).
 * @details Każdy wiersz rastra przecina krawędzie wielokąta. Posortowane przecięcia są
 * łączone w pary według reguły parzystości.
 */

#include "../PrimitiveRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

PrimitiveRenderer& PrimitiveRenderer::Polygon(std::span<const Point2D> vertices) {
    if (!IsSimplePolygon(vertices)) return *this; // łamana wiązana - nic nie rysujemy

    if (filled_) {
        ScanlineFill(vertices);
    } else {
        for (std::size_t i = 0; i < vertices.size(); ++i)
            Line(vertices[i], vertices[(i + 1) % vertices.size()]);
    }
    return *this;
}

PrimitiveRenderer& PrimitiveRenderer::Polygon(std::span<const LineSegment> edges) {
    if (const auto vertices = PolygonVertices(edges)) Polygon(*vertices);
    return *this;
}

/**
 * @details Algorytm „YX”. Dla każdej linii rastra (środek piksela y + 0.5) zbieramy punkty
 * przecięcia z krawędziami, sortujemy je po x i wypełniamy piksele między kolejnymi
 * parami - wewnątrz wielokąta jest się po nieparzystej liczbie przecięć.
 *
 * Problemy brzegowe rozwiązuje reguła półotwartego przedziału: krawędź liczy się dla linii,
 * gdy y_min <= y < y_max. Dzięki temu wierzchołek, przez który linia „przechodzi”, daje
 * jedno przecięcie, a wierzchołek będący lokalnym ekstremum - zero albo dwa, więc pary się
 * zgadzają. Krawędzie poziome nie przecinają linii rastra w jednym punkcie, więc są pomijane.
 */
void PrimitiveRenderer::ScanlineFill(std::span<const Point2D> vertices) {
    const auto [minIt, maxIt] = std::minmax_element(
        vertices.begin(), vertices.end(), [](Point2D a, Point2D b) { return a.Y() < b.Y(); });
    const int firstRow = static_cast<int>(std::floor(minIt->Y()));
    const int lastRow = static_cast<int>(std::ceil(maxIt->Y()));

    std::vector<float> crossings;
    for (int y = firstRow; y <= lastRow; ++y) {
        const float scan = static_cast<float>(y) + 0.5f;
        crossings.clear();

        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const Point2D a = vertices[i];
            const Point2D b = vertices[(i + 1) % vertices.size()];
            if (a.Y() == b.Y()) continue; // krawędź pozioma
            const float low = std::min(a.Y(), b.Y());
            const float high = std::max(a.Y(), b.Y());
            if (scan < low || scan >= high) continue;
            crossings.push_back(a.X() + (scan - a.Y()) * (b.X() - a.X()) / (b.Y() - a.Y()));
        }

        std::sort(crossings.begin(), crossings.end());
        for (std::size_t k = 0; k + 1 < crossings.size(); k += 2) {
            // piksele, których środek (x + 0.5) leży w przedziale [x1, x2)
            const int fromX = static_cast<int>(std::ceil(crossings[k] - 0.5f));
            const int toX = static_cast<int>(std::ceil(crossings[k + 1] - 0.5f)) - 1;
            if (fromX <= toX) Span(y, fromX, toX);
        }
    }
}
