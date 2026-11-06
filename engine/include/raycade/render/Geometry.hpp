/**
 * @file Geometry.hpp
 * @brief Klasy geometryczne Point2D i LineSegment oraz funkcje dla wielokątów.
 */
#pragma once

#include <raylib.h>

#include <optional>
#include <span>
#include <vector>

class PrimitiveRenderer;

/**
 * @brief Punkt dwuwymiarowy zgodny ze współrzędnymi raylib.
 * @details Zamienia się automatycznie z i na Vector2, więc można go podawać wszędzie tam,
 * gdzie silnik przyjmuje Vector2.
 * @code{.cpp}
 * Point2D p{100, 50};
 * p.X(120).Y(80).Draw(renderer);
 * @endcode
 */
class Point2D {
  public:
    /** @brief Tworzy punkt w początku układu. */
    constexpr Point2D() = default;

    /**
     * @brief Tworzy punkt z dwóch współrzędnych.
     * @param x Współrzędna pozioma.
     * @param y Współrzędna pionowa.
     */
    constexpr Point2D(float x, float y) : x_(x), y_(y) {}

    /**
     * @brief Tworzy punkt z wektora raylib.
     * @param vector Współrzędne wektora.
     */
    constexpr Point2D(Vector2 vector) : x_(vector.x), y_(vector.y) {}

    /**
     * @brief Konwertuje punkt na wektor raylib.
     * @return Wektor o tych samych współrzędnych.
     */
    constexpr operator Vector2() const { return {x_, y_}; }

    /**
     * @brief Pobiera współrzędną poziomą.
     * @return Wartość X.
     */
    float X() const { return x_; }

    /**
     * @brief Pobiera współrzędną pionową.
     * @return Wartość Y.
     */
    float Y() const { return y_; }

    /**
     * @brief Zmienia współrzędną poziomą.
     * @param x Nowa wartość X.
     * @return Ten sam punkt.
     */
    Point2D& X(float x) {
        x_ = x;
        return *this;
    }

    /**
     * @brief Zmienia współrzędną pionową.
     * @param y Nowa wartość Y.
     * @return Ten sam punkt.
     */
    Point2D& Y(float y) {
        y_ = y;
        return *this;
    }

    /**
     * @brief Rysuje punkt bieżącym kolorem renderera.
     * @param renderer Renderer docelowy.
     * @return Ten sam punkt.
     */
    const Point2D& Draw(PrimitiveRenderer& renderer) const;

    /** @brief Porównuje obie współrzędne punktów. */
    bool operator==(const Point2D&) const = default;

  private:
    float x_ = 0.0f; /**< Współrzędna pozioma. */
    float y_ = 0.0f; /**< Współrzędna pionowa. */
};

/**
 * @brief Odcinek wyznaczony przez dwa punkty Point2D.
 * @code{.cpp}
 * LineSegment edge{{0, 0}, {100, 50}};
 * edge.Draw(renderer);       // algorytm domyślny (raylib)
 * edge.Draw(renderer, true); // algorytm przyrostowy
 * @endcode
 */
class LineSegment {
  public:
    /** @brief Tworzy odcinek zerowy w początku układu. */
    constexpr LineSegment() = default;

    /**
     * @brief Tworzy odcinek między dwoma punktami.
     * @param from Początek.
     * @param to Koniec.
     */
    constexpr LineSegment(Point2D from, Point2D to) : from_(from), to_(to) {}

    /**
     * @brief Pobiera początek odcinka.
     * @return Punkt początkowy.
     */
    Point2D From() const { return from_; }

    /**
     * @brief Pobiera koniec odcinka.
     * @return Punkt końcowy.
     */
    Point2D To() const { return to_; }

    /**
     * @brief Zmienia początek odcinka.
     * @param from Nowy początek.
     * @return Ten sam odcinek.
     */
    LineSegment& From(Point2D from) {
        from_ = from;
        return *this;
    }

    /**
     * @brief Zmienia koniec odcinka.
     * @param to Nowy koniec.
     * @return Ten sam odcinek.
     */
    LineSegment& To(Point2D to) {
        to_ = to;
        return *this;
    }

    /**
     * @brief Sprawdza, czy odcinki mają choć jeden punkt wspólny.
     * @details Uwzględnia także styk końcami i nakładanie się odcinków współliniowych.
     * @param other Drugi odcinek.
     * @return true, gdy odcinki mają wspólny punkt.
     */
    bool Intersects(const LineSegment& other) const;

    /**
     * @brief Rysuje odcinek bieżącym kolorem renderera wybranym algorytmem.
     * @param renderer Renderer docelowy.
     * @param incremental true dla algorytmu przyrostowego, false dla domyślnego z raylib.
     * @return Ten sam odcinek.
     * @note Ustawia na rendererze flagę Incremental(incremental), która - jak cały stan
     * pędzla - obowiązuje także dla kolejnych wywołań, aż do jej zmiany lub Reset().
     */
    const LineSegment& Draw(PrimitiveRenderer& renderer, bool incremental = false) const;

  private:
    Point2D from_; /**< Początek odcinka. */
    Point2D to_;   /**< Koniec odcinka. */
};

/**
 * @brief Sprawdza, czy wierzchołki tworzą łamaną zwyczajną zamkniętą.
 * @details Wymaga co najmniej trzech wierzchołków, a krawędzie niesąsiednie nie mogą mieć
 * punktów wspólnych. W przeciwnym razie jest to łamana wiązana.
 * @param vertices Wierzchołki w kolejności obwodu.
 * @return false dla mniej niż trzech punktów lub przecięcia krawędzi niesąsiednich.
 * @note Powtórzone sąsiednie wierzchołki nie są osobno odrzucane.
 */
bool IsSimplePolygon(std::span<const Point2D> vertices);

/**
 * @brief Zamienia uporządkowany, zamknięty łańcuch odcinków na listę wierzchołków.
 * @details Koniec każdego odcinka musi być początkiem następnego, a koniec ostatniego -
 * początkiem pierwszego.
 * @param edges Odcinki w kolejności obwodu.
 * @return Lista wierzchołków albo std::nullopt, gdy łańcuch jest pusty lub niedomknięty.
 */
std::optional<std::vector<Point2D>> PolygonVertices(std::span<const LineSegment> edges);
