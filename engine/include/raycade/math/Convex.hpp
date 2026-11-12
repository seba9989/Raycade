/**
 * @file Convex.hpp
 * @brief Figury wypukłe do wykrywania kolizji: funkcja wsparcia, GJK, EPA i podział wielokąta.
 */
#pragma once

#include <raylib.h>

#include <optional>
#include <span>
#include <vector>

#include <raycade/math/AffineMatrix2D.hpp>
#include <raycade/math/Geometry.hpp>

/**
 * @brief Figura wypukła opisana funkcją wsparcia - wspólny język wszystkich kolizji.
 * @details Każdy kształt z @ref shapes rozkłada się na jedną albo kilka takich części:
 * - wielokąt wypukły (także odcinek - 2 punkty - i punkt - 1 punkt),
 * - elipsę (także okrąg) - środek i dwie półosie, które po obrocie i nierównej skali
 *   przestają być prostopadłe, a figura nadal jest elipsą.
 *
 * Algorytmy kolizji (GJK, EPA) nie znają rodzaju figury - pytają tylko o Support(), czyli
 * najdalszy punkt figury w danym kierunku. Dzięki temu jedna para funkcji obsługuje każdą
 * parę kształtów, zamiast osobnego testu dla każdej kombinacji.
 */
struct ConvexPart {
    /** @brief Rodzaj części. */
    enum class Kind {
        Polygon, /**< Wielokąt wypukły, odcinek albo punkt (pole points). */
        Ellipse, /**< Elipsa: center + axisX * cos(t) + axisY * sin(t). */
    };

    Kind kind = Kind::Polygon;   /**< Rodzaj części. */
    std::vector<Vector2> points; /**< Wierzchołki wielokąta w kolejności obwodu. */
    Vector2 center{};            /**< Środek elipsy. */
    Vector2 axisX{};             /**< Pierwsza półoś elipsy (wektor od środka). */
    Vector2 axisY{};             /**< Druga półoś elipsy (wektor od środka). */

    /** @brief Pojedynczy punkt. */
    static ConvexPart Point(Vector2 point);

    /** @brief Odcinek. */
    static ConvexPart Segment(Vector2 from, Vector2 to);

    /** @brief Wielokąt wypukły (wierzchołki w kolejności obwodu). */
    static ConvexPart Polygon(std::vector<Vector2> points);

    /**
     * @brief Elipsa o osiach równoległych do osi układu.
     * @param center Środek.
     * @param radius Promienie {poziomy, pionowy}; okrąg to {r, r}.
     */
    static ConvexPart Ellipse(Vector2 center, Vector2 radius);

    /**
     * @brief Najdalszy punkt figury w danym kierunku (funkcja wsparcia).
     * @param direction Kierunek (nie musi być znormalizowany).
     * @return Punkt figury o największym iloczynie skalarnym z kierunkiem.
     */
    Vector2 Support(Vector2 direction) const;

    /** @brief Środek figury (średnia wierzchołków albo środek elipsy). */
    Vector2 Center() const;

    /**
     * @brief Ta sama figura po przekształceniu afinicznym (np. z układu lokalnego do sceny).
     * @details Przekształcenie afiniczne zachowuje wypukłość, a elipsę zamienia w elipsę.
     */
    ConvexPart Transformed(const AffineMatrix2D& matrix) const;

    /**
     * @brief Punkty obwodu do narysowania (podgląd kolizji).
     * @param segments Liczba odcinków przybliżających elipsę.
     */
    std::vector<Point2D> Outline(int segments = 24) const;
};

/**
 * @brief Wnikanie dwóch figur: kierunek i odległość, o jaką trzeba je rozsunąć.
 */
struct Penetration {
    Vector2 normal{}; /**< Kierunek (jednostkowy), w którym trzeba przesunąć pierwszą figurę. */
    float depth = 0;  /**< Odległość przesunięcia, po której figury tylko się stykają. */
};

/**
 * @brief Sprawdza, czy dwie figury wypukłe mają część wspólną (algorytm GJK).
 * @details GJK szuka w różnicy Minkowskiego A - B (wszystkie wektory a - b) trójkąta
 * zawierającego początek układu. Jeśli go znajdzie, figury się przecinają. Samo stykanie
 * brzegami nie jest liczone jako przecięcie.
 * @return true, gdy figury nachodzą na siebie.
 */
bool ConvexOverlap(const ConvexPart& a, const ConvexPart& b);

/**
 * @brief Liczy wnikanie dwóch figur wypukłych (GJK, a potem EPA).
 * @details EPA (Expanding Polytope Algorithm) rozszerza trójkąt z GJK o kolejne punkty
 * wsparcia, aż znajdzie krawędź różnicy Minkowskiego najbliższą początkowi układu - jej
 * normalna to kierunek, a odległość - głębokość wnikania. Dla elips wynik jest przybliżony
 * (z dokładnością do ułamka piksela).
 * @return Wnikanie albo std::nullopt, gdy figury się nie przecinają.
 */
std::optional<Penetration> ConvexPenetration(const ConvexPart& a, const ConvexPart& b);

/**
 * @brief Sprawdza, czy wielokąt (łamana zwyczajna zamknięta) jest wypukły.
 * @param vertices Wierzchołki w kolejności obwodu.
 */
bool IsConvexPolygon(std::span<const Point2D> vertices);

/**
 * @brief Dzieli wielokąt prosty na trójkąty metodą obcinania uszu (ear clipping).
 * @details Wielokąt wypukły zwraca jako jedną część bez dzielenia.
 * @param vertices Wierzchołki wielokąta prostego w kolejności obwodu.
 * @return Części wypukłe; pusta lista, gdy wielokąt jest zdegenerowany.
 */
std::vector<std::vector<Point2D>> ConvexPieces(std::span<const Point2D> vertices);
