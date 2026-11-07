/**
 * @file Shapes.hpp
 * @brief Zbiorczy nagłówek wszystkich wbudowanych kształtów.
 * @details Wygodny punkt włączenia dla kodu scen - dołącza go Engine.hpp.
 */
#pragma once

// Tabela poniżej - formatter łamałby jej wiersze.
// clang-format off
/**
 * @namespace shapes
 * @brief Kształty, które można tworzyć przez Scene::Spawn() i Entity::Spawn().
 * @details W kodzie wystarczy wpisać `Spawn<shapes::`, aby edytor podpowiedział typy.
 *
 * | Typ                   | Co rysuje                             | Własne metody (settery i gettery)                           |
 * |-----------------------|---------------------------------------|-------------------------------------------------------------|
 * | @ref shapes::Point    | pojedynczy piksel                     | -                                                           |
 * | @ref shapes::Points   | chmurę punktów                        | `Positions()`                                               |
 * | @ref shapes::Line     | odcinek                               | `From()`, `To()`, `Incremental()`                           |
 * | @ref shapes::Lines    | niezależne odcinki z par wierzchołków | `Vertices()`, `Incremental()`                               |
 * | @ref shapes::Polyline | łamaną otwartą lub zamkniętą          | `Vertices()`, `Closed()`, `Incremental()`                   |
 * | @ref shapes::Rect     | prostokąt                             | `Size()`, `Incremental()`                                   |
 * | @ref shapes::Triangle | trójkąt                               | `Vertices()`, `Incremental()`                               |
 * | @ref shapes::Polygon  | wielokąt (wypełnianie scanline)       | `Vertices()`, `Edges()`, `Incremental()`, `IsSimple()`      |
 * | @ref shapes::Circle   | okrąg / koło                          | `Radius()`, `Symmetric()`                                   |
 * | @ref shapes::Ellipse  | elipsę                                | `Radius()`, `Symmetric()`                                   |
 * | @ref shapes::Canvas   | bitmapę w RAM (flood/boundary fill)   | `Size()`, `Background()`, `Clear()`, `Content()`, `Pixels()` |
 * | @ref shapes::Surface  | dzieci rysowane do tekstury GPU       | `Size()`, `Background()`, `Tint()`, `Smooth()`, `Target()`  |
 * | @ref shapes::Group    | nic - grupuje i przesuwa dzieci       | -                                                           |
 *
 * Pole tekstowe ui::TextField nie jest kształtem geometrycznym, dlatego leży w przestrzeni
 * nazw @ref ui, ale korzysta z tych samych klas bazowych.
 *
 * Metody wspólne ma każdy kształt (z klas Transformable, Drawable, Updatable i Entity):
 * - przekształcenia: Position(), Translate(), Rotate(), Scale(),
 * - wygląd: Paint(), Fill(), Layer(), Blend(),
 * - drzewo i układ: Spawn(), Children(), ScreenSpace(),
 * - zdarzenia: OnUpdate(), OnClick(), OnMouseUp(), OnMouseEnter(), OnMouseLeave(), OnScroll(),
 *   OnDragStart(), OnDrag(), OnDragEnd(), OnKeyPressed(), OnKeyReleased(),
 * - stan: IsHovered(), IsDragging(), IsFilled(), IsScreenSpace().
 *
 * Konwencje: setter i getter mają tę samą nazwę (`Size(v)` / `Size()`), getter stanu
 * logicznego ma prefiks `Is` (`IsIncremental()`), a punkty i wierzchołki są typu Point2D.
 * Fill() działa w Rect, Triangle, Circle, Ellipse i Polygon, a Incremental() (własny algorytm
 * odcinka) we wszystkich kształtach z konturem z odcinków: Line, Lines, Polyline, Polygon,
 * Rect i Triangle. Wszystkie metody konfiguracji (wspólne i własne kształtu) używają deducing
 * this i zwracają typ obiektu, na którym je wywołano - także klasy pochodnej, np.
 * `class Ball : public shapes::Circle` - więc można je łączyć w dowolnej kolejności.
 * Patrz @ref Entity.
 */
// clang-format on

// Nagłówek zbiorczy: dołączone pliki są częścią jego interfejsu (dla clangd/IWYU).
// IWYU pragma: begin_exports
#include "shapes/Canvas.hpp"
#include "shapes/Circle.hpp"
#include "shapes/Ellipse.hpp"
#include "shapes/Group.hpp"
#include "shapes/Line.hpp"
#include "shapes/Lines.hpp"
#include "shapes/Point.hpp"
#include "shapes/Points.hpp"
#include "shapes/Polygon.hpp"
#include "shapes/Polyline.hpp"
#include "shapes/Rect.hpp"
#include "shapes/Surface.hpp"
#include "shapes/Triangle.hpp"
// IWYU pragma: end_exports
