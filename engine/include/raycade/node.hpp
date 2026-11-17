/**
 * @file node.hpp
 * @brief Zbiorczy nagłówek wszystkiego, co gra tworzy przez Spawn<T>().
 * @details Dołączany przez `<raycade>`.
 */
#pragma once

// Tabela poniżej - formatter łamałby jej wiersze.
// clang-format off
/**
 * @namespace node
 * @brief Obiekty sceny tworzone przez Scene::Spawn() i Entity::Spawn().
 * @details W kodzie wystarczy wpisać `Spawn<node::`, aby edytor podpowiedział kategorie:
 *
 * | Przestrzeń nazw      | Typy                                                          |
 * |----------------------|---------------------------------------------------------------|
 * | node::Group          | niewidoczny rodzic do grupowania obiektów                     |
 * | @ref node::shapes    | figury geometryczne: Rect, Circle, Polygon, Line...           |
 * | @ref node::visual    | obrazy i tekst: Sprite, Text, Canvas, RenderGroup             |
 * | @ref node::ui        | elementy interfejsu: TextField                                |
 * | @ref node::physics   | ciała i obszary: StaticBody, CharacterBody, RigidBody, Area   |
 * | @ref node::collider  | kształty kolizji: Collider\<S\> i aliasy (collider::Rect...) |
 * | @ref node::sound     | dźwięk w świecie gry: Emitter, Listener                       |
 *
 * Klasy bazowe (Entity, Transformable, Drawable...) są poza tą przestrzenią nazw - dziedziczą
 * po nich także własne obiekty gry. Po `using namespace node;` wystarczy pisać
 * `Spawn<shapes::Circle>()`; nazwa `audio::` nadal oznacza mikser.
 */
// clang-format on

// IWYU pragma: begin_exports
#include <raycade/node/Group.hpp>
#include <raycade/node/physics.hpp>
#include <raycade/node/shapes.hpp>
#include <raycade/node/sound.hpp>
#include <raycade/node/ui.hpp>
#include <raycade/node/visual.hpp>
// IWYU pragma: end_exports
