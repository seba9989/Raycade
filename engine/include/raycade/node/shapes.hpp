/**
 * @file shapes.hpp
 * @brief Zbiorczy nagłówek figur geometrycznych.
 * @details Wygodny punkt włączenia dla kodu scen - dołącza go nagłówek zbiorczy `<raycade>`.
 */
#pragma once

// Tabela poniżej - formatter łamałby jej wiersze.
// clang-format off
/**
 * @namespace node::shapes
 * @brief Figury geometryczne, które można tworzyć przez Scene::Spawn() i Entity::Spawn().
 * @details W kodzie wystarczy wpisać `Spawn<node::shapes::`, aby edytor podpowiedział typy.
 * Wszystkie dziedziczą po node::shapes::Shape (wypełnienie Filled() i algorytm Raster()).
 *
 * | Typ                         | Co rysuje                             | Własne metody (settery i gettery)     |
 * |-----------------------------|---------------------------------------|---------------------------------------|
 * | @ref node::shapes::Point    | pojedynczy piksel                     | -                                     |
 * | @ref node::shapes::Points   | chmurę punktów                        | `Positions()`                         |
 * | @ref node::shapes::Line     | odcinek                               | `From()`, `To()`                      |
 * | @ref node::shapes::Lines    | niezależne odcinki z par wierzchołków | `Vertices()`                          |
 * | @ref node::shapes::Polyline | łamaną otwartą lub zamkniętą          | `Vertices()`, `Closed()`              |
 * | @ref node::shapes::Rect     | prostokąt                             | `Size()`                              |
 * | @ref node::shapes::Polygon  | wielokąt (wypełnianie scanline)       | `Vertices()`, `Edges()`, `IsSimple()` |
 * | @ref node::shapes::Triangle | trójkąt (wielokąt z 3 wierzchołkami)  | `Vertices(a, b, c)`                   |
 * | @ref node::shapes::Circle   | okrąg / koło                          | `Radius()`                            |
 * | @ref node::shapes::Ellipse  | elipsę                                | `Radius()`                            |
 *
 * Obiekty z obrazem i tekstem (Sprite, Text, Canvas, RenderGroup) są w przestrzeni nazw
 * @ref node::visual, pole tekstowe w @ref node::ui, a niewidoczny rodzic node::Group - bezpośrednio
 * w @ref node. Nie są figurami, więc nie mają Filled() ani Raster().
 *
 * Metody wspólne ma każda figura (z klas Shape, Transformable, Drawable, Updatable i Entity):
 * - przekształcenia: Position(), Translate(), Rotation(), Rotate(), Scale(), ScaleBy(),
 * - wygląd: Paint(), Filled(), Raster(), Visible(), DrawLayer(), Blend(), ClipRect(), Shader(),
 * - drzewo i układ: Spawn(), Children(), ScreenSpace(),
 * - zdarzenia: OnUpdate(), OnClick(), OnMouseReleased(), OnMouseEnter(), OnMouseLeave(), OnScroll(),
 *   OnDragStart(), OnDrag(), OnDragEnd(), OnKeyPressed(), OnKeyReleased(),
 * - stan: IsHovered(), IsDragging(), IsFilled(), IsVisible(), IsScreenSpace().
 *
 * Konwencje: setter i getter mają tę samą nazwę (`Size(v)` / `Size()`), getter stanu
 * logicznego ma prefiks `Is` (`IsFilled()`), a punkty i wierzchołki są typu Point2D.
 * Filled() działa w Rect, Triangle, Circle, Ellipse i Polygon, a Raster(Rasterizer::Custom)
 * (algorytmy z laboratorium) - w figurach z odcinków oraz w Circle i Ellipse; tabela jest
 * przy node::shapes::Shape. Wszystkie metody konfiguracji używają deducing this i zwracają typ
 * obiektu, na którym je wywołano - także klasy pochodnej, np.
 * `class Ball : public node::shapes::Circle` - więc można je łączyć w dowolnej kolejności.
 * Patrz @ref Entity.
 */
// clang-format on

// Nagłówek zbiorczy: dołączone pliki są częścią jego interfejsu (dla clangd/IWYU).
// IWYU pragma: begin_exports
#include <raycade/node/shapes/Shape.hpp>
#include <raycade/node/shapes/line/Line.hpp>
#include <raycade/node/shapes/line/Lines.hpp>
#include <raycade/node/shapes/line/Polyline.hpp>
#include <raycade/node/shapes/point/Point.hpp>
#include <raycade/node/shapes/point/Points.hpp>
#include <raycade/node/shapes/polygon/Polygon.hpp>
#include <raycade/node/shapes/polygon/Rect.hpp>
#include <raycade/node/shapes/polygon/Triangle.hpp>
#include <raycade/node/shapes/round/Circle.hpp>
#include <raycade/node/shapes/round/Ellipse.hpp>
// IWYU pragma: end_exports
