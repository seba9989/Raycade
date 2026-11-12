/**
 * @file visual.hpp
 * @brief Zbiorczy nagłówek obiektów z obrazem i tekstem.
 * @details Dołączany przez `<raycade>`.
 */
#pragma once

// Tabela poniżej - formatter łamałby jej wiersze.
// clang-format off
/**
 * @namespace visual
 * @brief Obiekty sceny z treścią rastrową: obrazy, napisy, piksele i grupy rysowane do tekstury.
 * @details W odróżnieniu od figur z @ref shapes nie są opisane geometrią, tylko obrazem - nie
 * mają więc Fill() ani Raster(). Dziedziczą wprost po Entity, więc mają te same przekształcenia,
 * zdarzenia, dzieci i łańcuchowanie co figury.
 *
 * | Typ                       | Co rysuje                               | Własne metody (wybrane)                                     |
 * |---------------------------|-----------------------------------------|-------------------------------------------------------------|
 * | @ref visual::Sprite       | klatkę z arkusza, z animacją            | `Sheet()` (SpriteSheet), `Animation()`, `Play()`, `Pause()`, `Stop()`, `Frame()`, `Fps()`, `Speed()`, `FlipX()`, `FlipY()`, `Origin()`, `OnFinish()`, `OnFrameEvent()` |
 * | @ref visual::Text         | napis z czcionką (polskie znaki)        | `Content()`, `FontSize()`, `Spacing()`, `Font()`, `Align()` |
 * | @ref visual::Canvas       | bitmapę w RAM (flood/boundary fill)     | `Size()`, `Background()`, `Clear()`, `Edit()`, `Pixels()`   |
 * | @ref visual::RenderGroup  | dzieci rysowane do tekstury GPU         | `Size()`, `Background()`, `Smooth()`, `Target()`            |
 *
 * Paint() to w nich kolor mnożący piksele (domyślnie WHITE - bez zmian,
 * `Fade(WHITE, 0.5f)` - półprzezroczystość).
 */
// clang-format on

// IWYU pragma: begin_exports
#include <raycade/visual/Canvas.hpp>
#include <raycade/visual/RenderGroup.hpp>
#include <raycade/visual/Sprite.hpp>
#include <raycade/visual/Text.hpp>
// IWYU pragma: end_exports
