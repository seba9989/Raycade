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

std::optional<ConvexRayHit> ConvexRaycast(const ConvexPart& part, Vector2 from, Vector2 to) {
    const Vector2 d = Sub(to, from);
    if (LengthSqr(d) < kEpsilon * kEpsilon) return std::nullopt;
    const auto unit = [](Vector2 v) {
        const float length = std::sqrt(LengthSqr(v));
        return length < kEpsilon ? Vector2{} : Scale(v, 1.0f / length);
    };

    if (part.kind == ConvexPart::Kind::Ellipse) {
        // układ, w którym elipsa center + axisX*cos + axisY*sin jest okręgiem jednostkowym
        const float det = Cross(part.axisX, part.axisY);
        if (std::fabs(det) < kEpsilon) return std::nullopt;
        const auto inverse = [&](Vector2 v) {
            return Vector2{
                (part.axisY.y * v.x - part.axisY.x * v.y) / det,
                (-part.axisX.y * v.x + part.axisX.x * v.y) / det
            };
        };
        const Vector2 q0 = inverse(Sub(from, part.center));
        const Vector2 qd = inverse(d);
        const float a = Dot(qd, qd);
        const float b = 2.0f * Dot(q0, qd);
        const float c = Dot(q0, q0) - 1.0f;
        if (c <= 0.0f) return std::nullopt; // początek wewnątrz
        const float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f) return std::nullopt;
        const float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
        if (t < 0.0f || t > 1.0f) return std::nullopt;
        // normalna: gradient |A^-1 (p - c)|^2, czyli A^-T q
        const Vector2 q = Add(q0, Scale(qd, t));
        const Vector2 normal{
            (part.axisY.y * q.x - part.axisX.y * q.y) / det,
            (-part.axisY.x * q.x + part.axisX.x * q.y) / det
        };
        return ConvexRayHit{t, unit(normal)};
    }

    const std::vector<Vector2>& points = part.points;
    if (points.size() == 2) {
        const Vector2 s = Sub(points[1], points[0]);
        const float denominator = Cross(d, s);
        if (std::fabs(denominator) < kEpsilon) return std::nullopt; // równoległe
        const Vector2 offset = Sub(points[0], from);
        const float t = Cross(offset, s) / denominator;
        const float u = Cross(offset, d) / denominator;
        if (t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f) return std::nullopt;
        Vector2 normal = unit({-s.y, s.x});
        if (Dot(normal, d) > 0.0f) normal = Negate(normal); // ku początkowi promienia
        return ConvexRayHit{t, normal};
    }
    if (points.size() < 3) return std::nullopt;

    // Cyrus-Beck: promień wewnątrz wszystkich półpłaszczyzn krawędzi
    float area = 0.0f;
    for (std::size_t i = 0; i < points.size(); ++i)
        area += Cross(points[i], points[(i + 1) % points.size()]);
    const float orientation = area >= 0.0f ? 1.0f : -1.0f;
    float enter = 0.0f;
    float exit = 1.0f;
    bool entered = false;
    Vector2 enterNormal{};
    for (std::size_t i = 0; i < points.size(); ++i) {
        const Vector2 a = points[i];
        const Vector2 edge = Sub(points[(i + 1) % points.size()], a);
        const Vector2 outward = Scale(Vector2{edge.y, -edge.x}, orientation);
        const float distance = Dot(outward, Sub(a, from)); // > 0: początek po wewnętrznej stronie
        const float speed = Dot(outward, d);
        if (std::fabs(speed) < kEpsilon) {
            if (distance < 0.0f) return std::nullopt; // równolegle, na zewnątrz krawędzi
            continue;
        }
        const float t = distance / speed;
        if (speed < 0.0f) {
            if (t > enter) {
                enter = t;
                enterNormal = outward;
                entered = true;
            }
        } else {
            exit = std::min(exit, t);
        }
        if (enter > exit) return std::nullopt;
    }
    if (!entered) return std::nullopt; // początek wewnątrz wielokąta
    return ConvexRayHit{enter, unit(enterNormal)};
}

ConvexMass ConvexMassProperties(const ConvexPart& part) {
    ConvexMass mass;
    if (part.kind == ConvexPart::Kind::Ellipse) {
        mass.area = PI * std::fabs(Cross(part.axisX, part.axisY));
        mass.centroid = part.center;
        mass.inertia = mass.area * (LengthSqr(part.axisX) + LengthSqr(part.axisY)) / 4.0f;
        return mass;
    }
    const std::vector<Vector2>& points = part.points;
    if (points.empty()) return mass;
    mass.centroid = part.Center();
    if (points.size() < 3) return mass;

    // trójkąty (p0, pi, pi+1); współrzędne względem p0 - mniejsze błędy zaokrągleń
    const Vector2 origin = points[0];
    float area = 0.0f;
    Vector2 centroid{};
    float inertia = 0.0f; // względem origin
    for (std::size_t i = 1; i + 1 < points.size(); ++i) {
        const Vector2 e1 = Sub(points[i], origin);
        const Vector2 e2 = Sub(points[i + 1], origin);
        const float cross = Cross(e1, e2);
        const float triangle = 0.5f * cross;
        area += triangle;
        centroid = Add(centroid, Scale(Add(e1, e2), triangle / 3.0f));
        inertia += cross / 12.0f * (Dot(e1, e1) + Dot(e1, e2) + Dot(e2, e2));
    }
    if (std::fabs(area) < kEpsilon) return mass;
    centroid = Scale(centroid, 1.0f / area);
    mass.area = std::fabs(area);
    mass.centroid = Add(origin, centroid);
    // twierdzenie Steinera: z punktu origin do środka ciężkości
    mass.inertia = std::fabs(inertia - area * LengthSqr(centroid));
    return mass;
}

namespace {

/** @brief Krawędź figury najbardziej zwrócona w danym kierunku (do obcinania punktów styku). */
struct Feature {
    Vector2 deepest{}; /**< Wierzchołek najdalej w kierunku. */
    Vector2 from{};    /**< Początek krawędzi. */
    Vector2 to{};      /**< Koniec krawędzi. */
};

Feature BestEdge(const std::vector<Vector2>& points, Vector2 direction) {
    const std::size_t count = points.size();
    std::size_t best = 0;
    for (std::size_t i = 1; i < count; ++i)
        if (Dot(points[i], direction) > Dot(points[best], direction)) best = i;
    if (count == 2) return {points[best], points[0], points[1]};
    const Vector2 vertex = points[best];
    const Vector2 next = points[(best + 1) % count];
    const Vector2 prev = points[(best + count - 1) % count];
    const auto unit = [](Vector2 v) {
        const float length = std::sqrt(LengthSqr(v));
        return length < kEpsilon ? v : Scale(v, 1.0f / length);
    };
    // krawędź bardziej prostopadła do kierunku
    if (std::fabs(Dot(unit(Sub(vertex, prev)), direction)) <=
        std::fabs(Dot(unit(Sub(next, vertex)), direction)))
        return {vertex, prev, vertex};
    return {vertex, vertex, next};
}

/** @brief Obcina odcinek do półpłaszczyzny dot(normal, p) >= offset. */
int Clip(Vector2 a, Vector2 b, Vector2 normal, float offset, std::array<Vector2, 2>& out) {
    int count = 0;
    const float da = Dot(normal, a) - offset;
    const float db = Dot(normal, b) - offset;
    if (da >= 0.0f) out[static_cast<std::size_t>(count++)] = a;
    if (db >= 0.0f) out[static_cast<std::size_t>(count++)] = b;
    if (da * db < 0.0f && count < 2) {
        const float t = da / (da - db);
        out[static_cast<std::size_t>(count++)] = Add(a, Scale(Sub(b, a), t));
    }
    return count;
}

} // namespace

int ConvexContacts(
    const ConvexPart& a,
    const ConvexPart& b,
    const Penetration& penetration,
    std::array<ContactPoint, 2>& points
) {
    const Vector2 n = penetration.normal; // od b do a
    const auto polygonal = [](const ConvexPart& part) {
        return part.kind == ConvexPart::Kind::Polygon && part.points.size() >= 2;
    };
    // z elipsą albo punktem - jeden punkt: najgłębszy punkt tej figury w drugiej
    if (!polygonal(a)) {
        points[0] = {a.Support(Negate(n)), penetration.depth};
        return 1;
    }
    if (!polygonal(b)) {
        points[0] = {b.Support(n), penetration.depth};
        return 1;
    }

    const Feature featureA = BestEdge(a.points, Negate(n)); // ściana a zwrócona do b
    const Feature featureB = BestEdge(b.points, n);         // ściana b zwrócona do a
    const auto unit = [](Vector2 v) {
        const float length = std::sqrt(LengthSqr(v));
        return length < kEpsilon ? v : Scale(v, 1.0f / length);
    };
    const Vector2 edgeA = unit(Sub(featureA.to, featureA.from));
    const Vector2 edgeB = unit(Sub(featureB.to, featureB.from));
    // krawędź odniesienia - bardziej prostopadła do normalnej; druga jest obcinana
    const bool referenceIsA = std::fabs(Dot(edgeA, n)) <= std::fabs(Dot(edgeB, n));
    const Feature& reference = referenceIsA ? featureA : featureB;
    const Feature& incident = referenceIsA ? featureB : featureA;
    const Vector2 along = referenceIsA ? edgeA : edgeB;

    std::array<Vector2, 2> clipped{};
    std::array<Vector2, 2> clipped2{};
    if (Clip(incident.from, incident.to, along, Dot(along, reference.from), clipped) < 2 ||
        Clip(clipped[0], clipped[1], Negate(along), -Dot(along, reference.to), clipped2) < 2) {
        points[0] = {a.Support(Negate(n)), penetration.depth};
        return 1;
    }

    // normalna ściany odniesienia, zwrócona do drugiej figury
    Vector2 faceNormal{-along.y, along.x};
    const Vector2 towardOther = referenceIsA ? Negate(n) : n;
    if (Dot(faceNormal, towardOther) < 0.0f) faceNormal = Negate(faceNormal);
    const float face = Dot(faceNormal, reference.deepest);

    int count = 0;
    for (const Vector2 point : clipped2) {
        const float depth = face - Dot(faceNormal, point); // punkt za ścianą odniesienia
        if (depth < 0.0f) continue;
        points[static_cast<std::size_t>(count++)] = {point, depth};
    }
    if (count == 0) {
        points[0] = {a.Support(Negate(n)), penetration.depth};
        return 1;
    }
    return count;
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
