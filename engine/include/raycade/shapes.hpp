/**
 * @file shapes.hpp
 * @brief Zbiorczy nagłówek figur geometrycznych.
 * @details Wygodny punkt włączenia dla kodu scen - dołącza go nagłówek zbiorczy `<raycade>`.
 */
#pragma once

// Tabela poniżej - formatter łamałby jej wiersze.
// clang-format off
/**
 * @namespace shapes
 * @brief Figury geometryczne, które można tworzyć przez Scene::Spawn() i Entity::Spawn().
 * @details W kodzie wystarczy wpisać `Spawn<shapes::`, aby edytor podpowiedział typy. Wszystkie
 * dziedziczą po shapes::Shape (wypełnienie Fill() i algorytm Raster()).
 *
 * | Typ                   | Co rysuje                             | Własne metody (settery i gettery)                 |
 * |-----------------------|---------------------------------------|---------------------------------------------------|
 * | @ref shapes::Point    | pojedynczy piksel                     | -                                                 |
 * | @ref shapes::Points   | chmurę punktów                        | `Positions()`                                     |
 * | @ref shapes::Line     | odcinek                               | `From()`, `To()`                                  |
 * | @ref shapes::Lines    | niezależne odcinki z par wierzchołków | `Vertices()`                                      |
 * | @ref shapes::Polyline | łamaną otwartą lub zamkniętą          | `Vertices()`, `Closed()`                          |
 * | @ref shapes::Rect     | prostokąt                             | `Size()`                                          |
 * | @ref shapes::Polygon  | wielokąt (wypełnianie scanline)       | `Vertices()`, `Edges()`, `IsSimple()`             |
 * | @ref shapes::Triangle | trójkąt (wielokąt z 3 wierzchołkami)  | `Vertices(a, b, c)`                               |
 * | @ref shapes::Circle   | okrąg / koło                          | `Radius()`                                        |
 * | @ref shapes::Ellipse  | elipsę                                | `Radius()`                                        |
 *
 * Obiekty z obrazem i tekstem (Sprite, Text, Canvas, RenderGroup) są w przestrzeni nazw
 * @ref visual, pole tekstowe w @ref ui, a niewidoczny rodzic Group - obok Entity, bez
 * przestrzeni nazw. Nie są figurami, więc nie mają Fill() ani Raster().
 *
 * Metody wspólne ma każda figura (z klas Shape, Transformable, Drawable, Updatable i Entity):
 * - przekształcenia: Position(), Translate(), Rotate(), Scale(),
 * - wygląd: Paint(), Fill(), Raster(), Layer(), Blend(), Clip(), Shader(),
 * - drzewo i układ: Spawn(), Children(), ScreenSpace(),
 * - zdarzenia: OnUpdate(), OnClick(), OnMouseUp(), OnMouseEnter(), OnMouseLeave(), OnScroll(),
 *   OnDragStart(), OnDrag(), OnDragEnd(), OnKeyPressed(), OnKeyReleased(),
 * - stan: IsHovered(), IsDragging(), IsFilled(), IsScreenSpace().
 *
 * Konwencje: setter i getter mają tę samą nazwę (`Size(v)` / `Size()`), getter stanu
 * logicznego ma prefiks `Is` (`IsFilled()`), a punkty i wierzchołki są typu Point2D.
 * Fill() działa w Rect, Triangle, Circle, Ellipse i Polygon, a Raster(Rasterizer::Custom)
 * (algorytmy z laboratorium) - w figurach z odcinków oraz w Circle i Ellipse; tabela jest
 * przy shapes::Shape. Wszystkie metody konfiguracji używają deducing this i zwracają typ
 * obiektu, na którym je wywołano - także klasy pochodnej, np.
 * `class Ball : public shapes::Circle` - więc można je łączyć w dowolnej kolejności.
 * Patrz @ref Entity.
 */
// clang-format on

// Nagłówek zbiorczy: dołączone pliki są częścią jego interfejsu (dla clangd/IWYU).
// IWYU pragma: begin_exports
#include <raycade/shapes/Circle.hpp>
#include <raycade/shapes/Ellipse.hpp>
#include <raycade/shapes/Line.hpp>
#include <raycade/shapes/Lines.hpp>
#include <raycade/shapes/Point.hpp>
#include <raycade/shapes/Points.hpp>
#include <raycade/shapes/Polygon.hpp>
#include <raycade/shapes/Polyline.hpp>
#include <raycade/shapes/Rect.hpp>
#include <raycade/shapes/Shape.hpp>
#include <raycade/shapes/Triangle.hpp>
// IWYU pragma: end_exports
