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
 * | Typ                   | Co rysuje                             | Własne metody                         |
 * |-----------------------|---------------------------------------|---------------------------------------|
 * | @ref shapes::Point    | pojedynczy piksel                     | -                                     |
 * | @ref shapes::Points   | chmurę punktów                        | `Positions()`                         |
 * | @ref shapes::Line     | odcinek                               | `From()`, `To()`                      |
 * | @ref shapes::Lines    | niezależne odcinki z par wierzchołków | `Vertices()`                          |
 * | @ref shapes::Polyline | łamaną otwartą lub zamkniętą          | `Vertices()`, `Closed()`              |
 * | @ref shapes::Rect     | prostokąt                             | `Size()`                              |
 * | @ref shapes::Surface  | dzieci rysowane do tekstury GPU       | `Size()`, `Background()`, `Tint()`    |
 * | @ref shapes::Triangle | trójkąt                               | `Vertices()`                          |
 * | @ref shapes::Circle   | okrąg / koło                          | `Radius()`, `Symmetric()`             |
 * | @ref shapes::Ellipse  | elipsę                                | `Radius()`, `Symmetric()`             |
 * | @ref shapes::Polygon  | wielokąt (wypełnianie scanline)       | `Vertices()`, `Edges()`               |
 * | @ref shapes::Canvas   | bitmapę w RAM (flood/boundary fill)   | `Size()`, `Background()`, `Clear()`, `Content()`, `Pixels()` |
 * | @ref shapes::Group    | nic - grupuje i przesuwa dzieci       | -                                     |
 *
 * Metody wspólne ma każdy kształt (z klas Transformable, Drawable, Updatable i Entity):
 * Position(), Translate(), Rotate(), Scale(), Paint(), Layer(), Fill(), Children()
 * oraz zdarzenia OnUpdate(), OnClick(), OnMouseEnter(), OnMouseLeave() i OnKeyPressed().
 * Fill() działa w Rect, Triangle, Circle, Ellipse i Polygon, a Incremental() (własny algorytm
 * odcinka) w Line, Lines, Polyline i Polygon. Wszystkie zwracają konkretny typ kształtu,
 * więc można je łączyć w dowolnej kolejności - patrz @ref Entity.
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
