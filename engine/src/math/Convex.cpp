/**
 * @file Convex.cpp
 * @brief Figury wypukłe (funkcja wsparcia, obrys) i podział wielokąta na części wypukłe.
 */
#include <raycade/math/Convex.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

Vector2 Add(Vector2 a, Vector2 b) {
    return {a.x + b.x, a.y + b.y};
}

Vector2 Sub(Vector2 a, Vector2 b) {
    return {a.x - b.x, a.y - b.y};
}

Vector2 Scale(Vector2 v, float s) {
    return {v.x * s, v.y * s};
}

float Dot(Vector2 a, Vector2 b) {
    return a.x * b.x + a.y * b.y;
}

/** @brief Składowa z iloczynu wektorowego (pole równoległoboku ze znakiem). */
float Cross(Vector2 a, Vector2 b) {
    return a.x * b.y - a.y * b.x;
}

constexpr float kEpsilon = 1e-6f;

} // namespace

ConvexPart ConvexPart::Point(Vector2 point) {
    return Polygon({point});
}

ConvexPart ConvexPart::Segment(Vector2 from, Vector2 to) {
    return Polygon({from, to});
}

ConvexPart ConvexPart::Polygon(std::vector<Vector2> points) {
    ConvexPart part;
    part.kind = Kind::Polygon;
    part.points = std::move(points);
    return part;
}

ConvexPart ConvexPart::Ellipse(Vector2 center, Vector2 radius) {
    ConvexPart part;
    part.kind = Kind::Ellipse;
    part.center = center;
    part.axisX = {radius.x, 0.0f};
    part.axisY = {0.0f, radius.y};
    return part;
}

Vector2 ConvexPart::Support(Vector2 direction) const {
    if (kind == Kind::Ellipse) {
        // punkt c + X cos t + Y sin t z największym d·X cos t + d·Y sin t
        const float x = Dot(direction, axisX);
        const float y = Dot(direction, axisY);
        const float length = std::sqrt(x * x + y * y);
        if (length < kEpsilon) return center;
        return Add(center, Add(Scale(axisX, x / length), Scale(axisY, y / length)));
    }
    if (points.empty()) return {};
    Vector2 best = points.front();
    float bestDot = Dot(best, direction);
    for (const Vector2 point : points) {
        const float value = Dot(point, direction);
        if (value > bestDot) {
            bestDot = value;
            best = point;
        }
    }
    return best;
}

Vector2 ConvexPart::Center() const {
    if (kind == Kind::Ellipse || points.empty()) return center;
    Vector2 sum{};
    for (const Vector2 point : points)
        sum = Add(sum, point);
    return Scale(sum, 1.0f / static_cast<float>(points.size()));
}

ConvexPart ConvexPart::Transformed(const AffineMatrix2D& matrix) const {
    ConvexPart result = *this;
    for (Vector2& point : result.points)
        point = matrix.Apply(point);
    result.center = matrix.Apply(center);
    // półosie to wektory - tylko część liniowa przekształcenia, bez przesunięcia
    const auto linear = [&matrix](Vector2 v) {
        return Vector2{matrix.a * v.x + matrix.c * v.y, matrix.b * v.x + matrix.d * v.y};
    };
    result.axisX = linear(axisX);
    result.axisY = linear(axisY);
    return result;
}

std::vector<Point2D> ConvexPart::Outline(int segments) const {
    std::vector<Point2D> outline;
    if (kind == Kind::Polygon) {
        outline.assign(points.begin(), points.end());
        return outline;
    }
    outline.reserve(static_cast<std::size_t>(segments));
    for (int i = 0; i < segments; ++i) {
        const float t = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        outline.emplace_back(
            Add(center, Add(Scale(axisX, std::cos(t)), Scale(axisY, std::sin(t))))
        );
    }
    return outline;
}

bool IsConvexPolygon(std::span<const Point2D> vertices) {
    const std::size_t count = vertices.size();
    if (count < 3) return false;
    int sign = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const Vector2 a = vertices[i];
        const Vector2 b = vertices[(i + 1) % count];
        const Vector2 c = vertices[(i + 2) % count];
        const float cross = Cross(Sub(b, a), Sub(c, b));
        if (std::fabs(cross) < kEpsilon) continue;
        const int current = cross > 0.0f ? 1 : -1;
        if (sign != 0 && current != sign) return false;
        sign = current;
    }
    return sign != 0;
}

std::vector<std::vector<Point2D>> ConvexPieces(std::span<const Point2D> vertices) {
    if (vertices.size() < 3) return {};
    if (IsConvexPolygon(vertices)) return {std::vector<Point2D>(vertices.begin(), vertices.end())};

    // kolejność przeciwna do ruchu wskazówek zegara (pole ze znakiem > 0)
    std::vector<Vector2> polygon(vertices.begin(), vertices.end());
    float area = 0.0f;
    for (std::size_t i = 0; i < polygon.size(); ++i)
        area += Cross(polygon[i], polygon[(i + 1) % polygon.size()]);
    if (std::fabs(area) < kEpsilon) return {};
    if (area < 0.0f) std::reverse(polygon.begin(), polygon.end());

    const auto inside = [](Vector2 p, Vector2 a, Vector2 b, Vector2 c) {
        return Cross(Sub(b, a), Sub(p, a)) >= 0.0f && Cross(Sub(c, b), Sub(p, b)) >= 0.0f &&
               Cross(Sub(a, c), Sub(p, c)) >= 0.0f;
    };

    std::vector<std::vector<Point2D>> triangles;
    // ucho: wierzchołek wypukły, którego trójkąt z sąsiadami nie zawiera innych wierzchołków
    while (polygon.size() > 3) {
        const std::size_t count = polygon.size();
        bool clipped = false;
        for (std::size_t i = 0; i < count; ++i) {
            const Vector2 prev = polygon[(i + count - 1) % count];
            const Vector2 current = polygon[i];
            const Vector2 next = polygon[(i + 1) % count];
            if (Cross(Sub(current, prev), Sub(next, current)) <= 0.0f) continue; // wklęsły
            bool ear = true;
            for (std::size_t j = 0; j < count && ear; ++j) {
                const Vector2 p = polygon[j];
                if (j == i || j == (i + 1) % count || j == (i + count - 1) % count) continue;
                if (inside(p, prev, current, next)) ear = false;
            }
            if (!ear) continue;
            triangles.push_back({prev, current, next});
            polygon.erase(polygon.begin() + static_cast<std::ptrdiff_t>(i));
            clipped = true;
            break;
        }
        if (!clipped) return {}; // wielokąt nie jest prosty
    }
    triangles.push_back({polygon[0], polygon[1], polygon[2]});
    return triangles;
}
