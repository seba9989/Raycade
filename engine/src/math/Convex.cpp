/**
 * @file Convex.cpp
 * @brief Funkcja wsparcia, GJK, EPA i podział wielokąta na części wypukłe.
 */
#include <raycade/math/Convex.hpp>

#include <algorithm>
#include <array>
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

Vector2 Negate(Vector2 v) {
    return {-v.x, -v.y};
}

float Dot(Vector2 a, Vector2 b) {
    return a.x * b.x + a.y * b.y;
}

/** @brief Składowa z iloczynu wektorowego (pole równoległoboku ze znakiem). */
float Cross(Vector2 a, Vector2 b) {
    return a.x * b.y - a.y * b.x;
}

float LengthSqr(Vector2 v) {
    return Dot(v, v);
}

/**
 * @brief Składowa wektora toward prostopadła do edge - kierunek od krawędzi w stronę toward.
 * @details To (edge x toward) x edge z GJK zapisane bez iloczynów wektorowych w 3D.
 */
Vector2 PerpendicularToward(Vector2 edge, Vector2 toward) {
    return Sub(Scale(toward, Dot(edge, edge)), Scale(edge, Dot(edge, toward)));
}

/** @brief Punkt wsparcia różnicy Minkowskiego A - B. */
Vector2 MinkowskiSupport(const ConvexPart& a, const ConvexPart& b, Vector2 direction) {
    return Sub(a.Support(direction), b.Support(Negate(direction)));
}

constexpr float kEpsilon = 1e-6f;
constexpr int kMaxIterations = 64;

/** @brief Sympleks GJK: punkt, odcinek albo trójkąt (najnowszy punkt jest ostatni). */
struct Simplex {
    std::array<Vector2, 3> points{};
    int size = 0;

    void Push(Vector2 point) { points[static_cast<std::size_t>(size++)] = point; }
};

/**
 * @brief Upraszcza sympleks do części najbliższej początkowi układu i wybiera nowy kierunek.
 * @return true, gdy sympleks zawiera początek układu (figury się przecinają).
 */
bool NextSimplex(Simplex& simplex, Vector2& direction) {
    if (simplex.size == 2) {
        const Vector2 a = simplex.points[1];
        const Vector2 b = simplex.points[0];
        const Vector2 ab = Sub(b, a);
        const Vector2 ao = Negate(a);
        if (Dot(ab, ao) > 0.0f) {
            direction = PerpendicularToward(ab, ao);
            // początek układu leży na odcinku - figury stykają się albo nachodzą
            return LengthSqr(direction) <= kEpsilon * kEpsilon * LengthSqr(ab) * LengthSqr(ab);
        }
        simplex.points[0] = a;
        simplex.size = 1;
        direction = ao;
        return false;
    }

    // trójkąt c, b, a (a - najnowszy)
    const Vector2 a = simplex.points[2];
    const Vector2 b = simplex.points[1];
    const Vector2 c = simplex.points[0];
    const Vector2 ab = Sub(b, a);
    const Vector2 ac = Sub(c, a);
    const Vector2 ao = Negate(a);
    const Vector2 abPerp = PerpendicularToward(ab, Negate(ac)); // od krawędzi ab, z dala od c
    const Vector2 acPerp = PerpendicularToward(ac, Negate(ab)); // od krawędzi ac, z dala od b
    if (Dot(abPerp, ao) > 0.0f) {
        simplex.points[0] = b;
        simplex.points[1] = a;
        simplex.size = 2;
        direction = abPerp;
        return false;
    }
    if (Dot(acPerp, ao) > 0.0f) {
        simplex.points[0] = c;
        simplex.points[1] = a;
        simplex.size = 2;
        direction = acPerp;
        return false;
    }
    return true;
}

/**
 * @brief Algorytm GJK.
 * @param simplex Ostatni sympleks - przy przecięciu zawiera początek układu (dla EPA).
 * @return true, gdy figury się przecinają.
 */
bool Gjk(const ConvexPart& a, const ConvexPart& b, Simplex& simplex) {
    Vector2 direction = Sub(a.Center(), b.Center());
    if (LengthSqr(direction) < kEpsilon) direction = {1.0f, 0.0f};
    simplex.size = 0;
    simplex.Push(MinkowskiSupport(a, b, direction));
    direction = Negate(simplex.points[0]);
    for (int i = 0; i < kMaxIterations; ++i) {
        // początek układu w punkcie sympleksu - figury się tylko stykają
        if (LengthSqr(direction) < kEpsilon * kEpsilon) return false;
        const Vector2 point = MinkowskiSupport(a, b, direction);
        // nowy punkt nie przekracza początku układu - różnica Minkowskiego go nie zawiera
        if (Dot(point, direction) <= 0.0f) return false;
        simplex.Push(point);
        if (NextSimplex(simplex, direction)) return true;
    }
    return false;
}

/**
 * @brief Uzupełnia zdegenerowany sympleks (odcinek przez początek układu) do trójkąta.
 * @return false, gdy różnica Minkowskiego jest płaska (np. dwa odcinki na jednej prostej).
 */
bool CompleteTriangle(const ConvexPart& a, const ConvexPart& b, Simplex& simplex) {
    if (simplex.size == 3) return true;
    if (simplex.size != 2) return false;
    const Vector2 edge = Sub(simplex.points[1], simplex.points[0]);
    for (const Vector2 normal : {Vector2{-edge.y, edge.x}, Vector2{edge.y, -edge.x}}) {
        const Vector2 point = MinkowskiSupport(a, b, normal);
        if (Dot(point, normal) > kEpsilon * std::sqrt(LengthSqr(normal))) {
            simplex.Push(point);
            return true;
        }
    }
    return false;
}

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

bool ConvexOverlap(const ConvexPart& a, const ConvexPart& b) {
    Simplex simplex;
    return Gjk(a, b, simplex);
}

std::optional<Penetration> ConvexPenetration(const ConvexPart& a, const ConvexPart& b) {
    Simplex simplex;
    if (!Gjk(a, b, simplex) || !CompleteTriangle(a, b, simplex)) return std::nullopt;

    // wielokąt EPA zgodnie z ruchem przeciwnym do wskazówek zegara (normalne krawędzi na zewnątrz);
    // tablica stałego rozmiaru, bo każda iteracja dokłada najwyżej jeden punkt
    std::array<Vector2, kMaxIterations + 3> polytope{};
    std::size_t size = 3;
    std::copy(simplex.points.begin(), simplex.points.end(), polytope.begin());
    if (Cross(Sub(polytope[1], polytope[0]), Sub(polytope[2], polytope[0])) < 0.0f)
        std::swap(polytope[1], polytope[2]);

    Vector2 normal{};
    float distance = 0.0f;
    for (int iteration = 0; iteration < kMaxIterations; ++iteration) {
        // krawędź najbliższa początkowi układu
        std::size_t closest = 0;
        distance = INFINITY;
        for (std::size_t i = 0; i < size; ++i) {
            const Vector2 from = polytope[i];
            const Vector2 edge = Sub(polytope[(i + 1) % size], from);
            const float length = std::sqrt(LengthSqr(edge));
            if (length < kEpsilon) continue;
            const Vector2 outward{edge.y / length, -edge.x / length};
            const float edgeDistance = Dot(outward, from);
            if (edgeDistance < distance) {
                distance = edgeDistance;
                normal = outward;
                closest = i;
            }
        }
        if (!std::isfinite(distance)) return std::nullopt;

        // dalszego punktu w kierunku tej krawędzi nie ma - to brzeg różnicy Minkowskiego
        const Vector2 support = MinkowskiSupport(a, b, normal);
        if (Dot(support, normal) - distance < 1e-3f) break;
        // wstawienie punktu między końce najbliższej krawędzi
        std::copy_backward(
            polytope.begin() + static_cast<std::ptrdiff_t>(closest + 1),
            polytope.begin() + static_cast<std::ptrdiff_t>(size),
            polytope.begin() + static_cast<std::ptrdiff_t>(size + 1)
        );
        polytope[closest + 1] = support;
        ++size;
    }
    if (distance <= 0.0f) return std::nullopt; // tylko styk
    // przesunięcie A o -normal * distance wypycha początek układu na brzeg A - B
    return Penetration{Negate(normal), distance};
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
