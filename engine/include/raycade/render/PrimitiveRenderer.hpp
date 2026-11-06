/**
 * @file PrimitiveRenderer.hpp
 * @brief Klasa PrimitiveRenderer - rysowanie prymitywów 2D na ekranie lub bitmapie.
 * @details Implementacja jest podzielona według obszaru: Primitives.cpp (piksele, odcinki,
 * prostokąty, trójkąty, łamane), RoundShapes.cpp (okręgi i elipsy), Polygon.cpp
 * (wielokąty i scanline) oraz Fill.cpp (wypełnianie przez spójność i Blit).
 */
#pragma once

#include <raylib.h>

#include <span>

#include "Bitmap.hpp"
#include "Geometry.hpp"

/**
 * @brief Renderer prymitywów dwuwymiarowych ze stanem podobnym do pędzla.
 * @details Stan obejmuje kolor, wypełnienie, wybór algorytmów oraz cel rysowania:
 * ekran lub Bitmap. Metody zwracają referencję do tego samego obiektu, więc wywołania
 * można łączyć. Stan zostaje między wywołaniami - Reset() przywraca domyślny pędzel,
 * ale nie zmienia celu rysowania (robią to tylko Target() i TargetScreen()), więc wywołany
 * w trakcie rysowania po bitmapie nie przeniesie przypadkiem rysowania na ekran.
 * @code{.cpp}
 * renderer.Paint(RED).Fill().Circle({100, 100}, 40)
 *         .Fill(false).Rect({200, 60}, {120, 80});
 * @endcode
 */
class PrimitiveRenderer {
  public:
    /** @name Stan pędzla */
    /** @{ */

    /**
     * @brief Ustawia bieżący kolor rysowania.
     * @param color Kolor raylib.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Paint(Color color) {
        color_ = color;
        return *this;
    }

    /**
     * @brief Włącza lub wyłącza wypełnienie kolejnych figur.
     * @param value Czy rysować wnętrze.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Fill(bool value = true) {
        filled_ = value;
        return *this;
    }

    /**
     * @brief Wybiera własny algorytm przyrostowy dla odcinków.
     * @details Dotyczy Line(), Lines(), Polyline() i konturu Polygon().
     * @param value true dla algorytmu własnego, false dla raylib.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Incremental(bool value = true) {
        incremental_ = value;
        return *this;
    }

    /**
     * @brief Wybiera własny algorytm z symetrią dla okręgów i elips.
     * @details Okrąg korzysta z symetrii ośmiokrotnej, elipsa - z czterokrotnej.
     * @param value true dla algorytmu własnego, false dla raylib.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Symmetric(bool value = true) {
        symmetric_ = value;
        return *this;
    }

    /**
     * @brief Przywraca czarny kolor, brak wypełnienia i algorytmy raylib.
     * @details Nie zmienia celu rysowania.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Reset();

    /** @} */

    /** @name Cel rysowania */
    /** @{ */

    /**
     * @brief Kieruje następne operacje rysowania do bitmapy w RAM.
     * @param bitmap Bitmapa docelowa.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Target(Bitmap& bitmap) {
        target_ = &bitmap;
        return *this;
    }

    /**
     * @brief Kieruje następne operacje rysowania na ekran (cel domyślny).
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& TargetScreen() {
        target_ = nullptr;
        return *this;
    }

    /** @} */

    /** @name Pojedyncze prymitywy */
    /** @{ */

    /**
     * @brief Rysuje pojedynczy piksel.
     * @param position Pozycja piksela.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Point(Vector2 position);

    /**
     * @brief Rysuje odcinek wybranym algorytmem (patrz Incremental()).
     * @param from Początek.
     * @param to Koniec.
     * @return Ten sam renderer.
     * @see LineIncremental() - opis algorytmu przyrostowego.
     */
    PrimitiveRenderer& Line(Vector2 from, Vector2 to);

    /**
     * @brief Rysuje kontur lub wypełnienie prostokąta.
     * @param position Lewy górny róg.
     * @param size Szerokość i wysokość.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Rect(Vector2 position, Vector2 size);

    /**
     * @brief Rysuje okrąg albo koło wybranym algorytmem (patrz Symmetric()).
     * @param center Środek.
     * @param radius Promień.
     * @return Ten sam renderer.
     * @see CircleSymmetric() - opis algorytmu z symetrią ośmiokrotną.
     */
    PrimitiveRenderer& Circle(Vector2 center, float radius);

    /**
     * @brief Rysuje elipsę wybranym algorytmem (patrz Symmetric()).
     * @details Na bitmapie używany jest zawsze algorytm własny, także gdy Symmetric(false),
     * bo raylib nie ma funkcji rysującej elipsę na obrazie w RAM.
     * @param center Środek.
     * @param radius Promienie {poziomy, pionowy}.
     * @return Ten sam renderer.
     * @see EllipseSymmetric() - opis algorytmu z symetrią czterokrotną.
     */
    PrimitiveRenderer& Ellipse(Vector2 center, Vector2 radius);

    /**
     * @brief Rysuje kontur lub wypełnienie trójkąta.
     * @param a Pierwszy wierzchołek.
     * @param b Drugi wierzchołek.
     * @param c Trzeci wierzchołek.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Triangle(Vector2 a, Vector2 b, Vector2 c);

    /** @} */

    /** @name Wielokąty */
    /** @{ */

    /**
     * @brief Rysuje wielokąt (łamaną zwyczajną zamkniętą) z uporządkowanych wierzchołków.
     * @details Gdy krawędzie niesąsiednie się przecinają (łamana wiązana), nic nie jest
     * rysowane - można to sprawdzić wcześniej przez IsSimplePolygon(). Wypełnienie
     * (Fill()) korzysta z algorytmu scanline z kontrolą parzystości.
     * @param vertices Wierzchołki w kolejności obwodu.
     * @return Ten sam renderer.
     * @see ScanlineFill() - opis wypełniania z kontrolą parzystości.
     */
    PrimitiveRenderer& Polygon(std::span<const Point2D> vertices);

    /**
     * @brief Rysuje wielokąt z uporządkowanego, zamkniętego łańcucha odcinków.
     * @details Niedomknięty lub samoprzecinający się łańcuch nie jest rysowany.
     * @param edges Odcinki w kolejności obwodu.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Polygon(std::span<const LineSegment> edges);

    /** @} */

    /** @name Wersje tablicowe (wiele prymitywów w jednej pętli) */
    /** @{ */

    /**
     * @brief Rysuje zbiór punktów.
     * @param positions Pozycje punktów.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Points(std::span<const Vector2> positions);

    /**
     * @brief Rysuje niezależne odcinki z par kolejnych wierzchołków: (0,1), (2,3), ...
     * @param vertices Końce odcinków; ostatni punkt bez pary jest pomijany.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Lines(std::span<const Vector2> vertices);

    /**
     * @brief Rysuje łamaną otwartą lub zamkniętą z wektorów raylib.
     * @details W odróżnieniu od Polygon() nie sprawdza przecięć - łamana może być wiązana.
     * @param vertices Wierzchołki w kolejności.
     * @param closed Czy połączyć ostatni wierzchołek z pierwszym.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Polyline(std::span<const Vector2> vertices, bool closed = false);

    /**
     * @brief Rysuje łamaną otwartą lub zamkniętą z punktów Point2D.
     * @details W odróżnieniu od Polygon() nie sprawdza przecięć.
     * @param vertices Wierzchołki w kolejności.
     * @param closed Czy połączyć ostatni wierzchołek z pierwszym.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Polyline(std::span<const Point2D> vertices, bool closed = false);

    /**
     * @brief Rysuje łamaną ze zbioru odcinków.
     * @details Łamana jest otwarta lub zamknięta zależnie od samych odcinków; ich ciągłość
     * ani przecięcia nie są sprawdzane.
     * @param segments Zbiór odcinków.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Polyline(std::span<const LineSegment> segments);

    /** @} */

    /**
     * @name Wypełnianie przez spójność
     * Wersje iteracyjne ze stosem (sąsiedztwo 4-punktowe), malujące bieżącym kolorem.
     * Wymagają celu-bitmapy, bo z ekranu nie da się odczytać koloru piksela.
     */
    /** @{ */

    /**
     * @brief Wypełnia obszar aż do pikseli w kolorze krawędzi (boundary fill).
     * @details Wnętrze może być różnokolorowe - granicą jest wyłącznie kolor krawędzi.
     * Piksel w kolorze wypełnienia jest traktowany jak już wypełniony.
     * @param seed Piksel startowy (ziarno).
     * @param boundary Kolor krawędzi.
     * @return Ten sam renderer; bez ustawionej bitmapy niczego nie zmienia.
     */
    PrimitiveRenderer& BoundaryFill(Vector2 seed, Color boundary);

    /**
     * @brief Wypełnia spójny obszar w kolorze piksela startowego (flood fill).
     * @details Granicą jest każdy kolor inny niż kolor ziarna.
     * @param seed Piksel startowy (ziarno).
     * @return Ten sam renderer; bez ustawionej bitmapy lub dla ziarna poza nią niczego
     * nie zmienia.
     */
    PrimitiveRenderer& FloodFill(Vector2 seed);

    /** @} */

    /** @name Wyświetlanie bitmap */
    /** @{ */

    /**
     * @brief Wyświetla bitmapę na ekranie jako teksturę.
     * @param bitmap Bitmapa źródłowa.
     * @param position Lewy górny róg na ekranie.
     * @return Ten sam renderer.
     */
    PrimitiveRenderer& Blit(const Bitmap& bitmap, Vector2 position);

    /** @} */

  private:
    /**
     * @brief Stawia pojedynczy piksel w bieżącym celu (ekran albo bitmapa).
     * @param x Współrzędna pozioma.
     * @param y Współrzędna pionowa.
     */
    void Plot(int x, int y);

    /**
     * @brief Rysuje poziomy odcinek pikseli (używany przez wypełnienia).
     * @param y Wiersz.
     * @param fromX Pierwsza kolumna.
     * @param toX Ostatnia kolumna (kolejność końców nie ma znaczenia).
     */
    void Span(int y, int fromX, int toX);

    /**
     * @brief Rysuje odcinek własnym algorytmem przyrostowym.
     * @param from Początek.
     * @param to Koniec.
     */
    void LineIncremental(Vector2 from, Vector2 to);

    /**
     * @brief Rysuje okrąg lub koło własnym algorytmem z symetrią ośmiokrotną.
     * @param center Środek.
     * @param radius Promień.
     */
    void CircleSymmetric(Vector2 center, float radius);

    /**
     * @brief Rysuje elipsę własnym algorytmem z symetrią czterokrotną.
     * @param center Środek.
     * @param radius Promienie {poziomy, pionowy}.
     */
    void EllipseSymmetric(Vector2 center, Vector2 radius);

    /**
     * @brief Wypełnia wielokąt algorytmem scanline z kontrolą parzystości.
     * @param vertices Wierzchołki w kolejności obwodu.
     */
    void ScanlineFill(std::span<const Point2D> vertices);

    Color color_ = BLACK;      /**< Bieżący kolor. */
    bool filled_ = false;      /**< Czy figury są wypełniane. */
    bool incremental_ = false; /**< Czy odcinki rysuje algorytm przyrostowy. */
    bool symmetric_ = false;   /**< Czy okręgi i elipsy rysuje algorytm z symetrią. */
    Bitmap* target_ = nullptr; /**< Bitmapa docelowa; nullptr oznacza ekran. */
};
