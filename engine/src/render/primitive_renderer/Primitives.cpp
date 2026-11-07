/**
 * @file Primitives.cpp
 * @brief Stan pędzla, podstawowe prymitywy, łamane i algorytm przyrostowy odcinka.
 */

#include <raycade/render/PrimitiveRenderer.hpp>

#include "Pixel.hpp"

#include <cmath>
#include <utility>

using pixel::Round;

PrimitiveRenderer& PrimitiveRenderer::Reset() {
    color_ = BLACK;
    filled_ = false;
    incremental_ = false;
    symmetric_ = false;
    return *this;
}

void PrimitiveRenderer::Plot(int x, int y) {
    if (target_)
        target_->Pixel(x, y, color_);
    else
        ::DrawPixel(x, y, color_);
}

void PrimitiveRenderer::Span(int y, int fromX, int toX) {
    if (fromX > toX) std::swap(fromX, toX);
    for (int x = fromX; x <= toX; ++x)
        Plot(x, y);
}

PrimitiveRenderer& PrimitiveRenderer::Point(Point2D position) {
    Plot(Round(position.X()), Round(position.Y()));
    return *this;
}

PrimitiveRenderer& PrimitiveRenderer::Line(Point2D from, Point2D to) {
    if (incremental_)
        LineIncremental(from, to);
    else if (target_)
        ::ImageDrawLineV(&target_->Edit(), from, to, color_);
    else
        ::DrawLineV(from, to, color_);
    return *this;
}

/**
 * @details Algorytm przyrostowy:
 * 1. m = dy/dx,
 * 2. x rośnie o 1 od lewej strony, a y zamiast liczenia m(x - x0) + y0 rośnie o m
 *    (y_{i+1} = y_i + m), piksel w (x, round(y)).
 *
 * Dla |m| > 1 role osi są zamieniane (krok o 1 po y od góry, x += 1/m), inaczej odcinek
 * miałby dziury. Odcinek zdegenerowany do punktu rysowany jest jako jeden piksel.
 */
void PrimitiveRenderer::LineIncremental(Vector2 from, Vector2 to) {
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;

    if (dx == 0.0f && dy == 0.0f) { // odcinek zdegenerowany do punktu
        Plot(Round(from.x), Round(from.y));
        return;
    }

    if (std::fabs(dx) >= std::fabs(dy)) { // |m| <= 1 -> krok po x
        if (from.x > to.x) std::swap(from, to);
        const float m = (to.y - from.y) / (to.x - from.x);
        float y = from.y;
        for (int x = Round(from.x); x <= Round(to.x); ++x) {
            Plot(x, Round(y));
            y += m;
        }
    } else { // |m| > 1 -> krok po y
        if (from.y > to.y) std::swap(from, to);
        const float mInverse = (to.x - from.x) / (to.y - from.y);
        float x = from.x;
        for (int y = Round(from.y); y <= Round(to.y); ++y) {
            Plot(Round(x), y);
            x += mInverse;
        }
    }
}

PrimitiveRenderer& PrimitiveRenderer::Rect(Point2D position, Vector2 size) {
    const Rectangle rectangle{position.X(), position.Y(), size.x, size.y};
    if (incremental_ && !filled_) {
        // kontur z czterech odcinków, jak Polygon() - piksele [x, x + w - 1] x [y, y + h - 1],
        // czyli te same co w konturze raylib
        const float right = position.X() + size.x - 1.0f;
        const float bottom = position.Y() + size.y - 1.0f;
        const Point2D corners[] = {
            position, {right, position.Y()}, {right, bottom}, {position.X(), bottom}};
        return Polyline(corners, true);
    }
    if (target_) {
        if (filled_)
            ::ImageDrawRectangleRec(&target_->Edit(), rectangle, color_);
        else
            ::ImageDrawRectangleLines(&target_->Edit(), rectangle, 1, color_);
    } else {
        if (filled_)
            ::DrawRectangleRec(rectangle, color_);
        else
            ::DrawRectangleLinesEx(rectangle, 1.0f, color_);
    }
    return *this;
}

PrimitiveRenderer& PrimitiveRenderer::Triangle(Point2D a, Point2D b, Point2D c) {
    if (filled_) {
        // raylib wymaga wierzchołków przeciwnie do ruchu wskazówek zegara
        const float cross = (b.X() - a.X()) * (c.Y() - a.Y()) - (b.Y() - a.Y()) * (c.X() - a.X());
        if (cross > 0.0f) std::swap(b, c);
        if (target_)
            ::ImageDrawTriangle(&target_->Edit(), a, b, c, color_);
        else
            ::DrawTriangle(a, b, c, color_);
    } else if (incremental_) {
        const Point2D corners[] = {a, b, c};
        Polyline(corners, true);
    } else {
        if (target_)
            ::ImageDrawTriangleLines(&target_->Edit(), a, b, c, color_);
        else
            ::DrawTriangleLines(a, b, c, color_);
    }
    return *this;
}

PrimitiveRenderer& PrimitiveRenderer::Points(std::span<const Point2D> positions) {
    for (const Point2D position : positions)
        Point(position);
    return *this;
}

PrimitiveRenderer& PrimitiveRenderer::Lines(std::span<const Point2D> vertices) {
    for (std::size_t i = 0; i + 1 < vertices.size(); i += 2)
        Line(vertices[i], vertices[i + 1]);
    return *this;
}

PrimitiveRenderer& PrimitiveRenderer::Polyline(std::span<const Point2D> vertices, bool closed) {
    if (vertices.size() < 2) return *this;
    for (std::size_t i = 0; i + 1 < vertices.size(); ++i)
        Line(vertices[i], vertices[i + 1]);
    if (closed) Line(vertices.back(), vertices.front());
    return *this;
}

PrimitiveRenderer& PrimitiveRenderer::Polyline(std::span<const LineSegment> segments) {
    for (const LineSegment& segment : segments)
        Line(segment.From(), segment.To());
    return *this;
}
