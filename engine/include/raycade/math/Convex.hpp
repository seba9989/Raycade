/**
 * @file Convex.hpp
 * @brief Figury wypukłe opisujące kształty kolizji i podział wielokąta na części wypukłe.
 */
#pragma once

#include <raylib.h>

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
 * Fizyka (Box2D) buduje z tych części swoje kształty, a podgląd kolizji rysuje ich obrys.
 * Support() - najdalszy punkt figury w danym kierunku - wystarcza do opisania każdej z nich.
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
