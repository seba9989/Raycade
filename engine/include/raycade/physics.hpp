/**
 * @file physics.hpp
 * @brief Zbiorczy nagłówek kolizji: collidery, ciała i obszary.
 * @details Dołączany przez `<raycade>`.
 */
#pragma once

// Tabela poniżej - formatter łamałby jej wiersze.
// clang-format off
/**
 * @page fizyka Kolizje
 * | Typ                 | Odpowiednik (Godot) | Rola                                                  |
 * |---------------------|---------------------|-------------------------------------------------------|
 * | Collider\<S\>       | CollisionShape2D    | kształt kolizji o kształcie figury S z @ref shapes    |
 * | StaticBody          | StaticBody2D        | ściany, podłogi, platformy                            |
 * | CharacterBody       | CharacterBody2D     | postać: Velocity() i MoveAndSlide()                   |
 * | Area                | Area2D              | wykrywanie: monety, kolce, strefy                     |
 * | RigidBody           | RigidBody2D         | bryły: skrzynie, piłki - masa, odbicia, tarcie, obrót |
 *
 * Collider jest dzieckiem ciała albo obszaru (także dalszym potomkiem). Fizyka działa w kroku
 * stałym (Engine::FixedStep(), domyślnie 1/60 s): FixedTick() i OnFixedUpdate() obiektów,
 * a na końcu kroku - zdarzenia zderzeń i obszarów. Ustawienia sceny: Scene::Gravity()
 * i Scene::DebugColliders(). Kolizje wykrywa algorytm GJK, a głębokość wnikania - EPA
 * (patrz ConvexPart). Bryły sztywne liczy solver impulsowy (jak w Box2D): punkty styku
 * z obcinania krawędzi, iteracje impulsów nacisku i tarcia z warm startingiem, a korekta
 * wnikania osobnymi pseudo-prędkościami (split impulse).
 */
// clang-format on

// IWYU pragma: begin_exports
#include <raycade/physics/Area.hpp>
#include <raycade/physics/CharacterBody.hpp>
#include <raycade/physics/Collider.hpp>
#include <raycade/physics/CollisionObject.hpp>
#include <raycade/physics/CollisionShape.hpp>
#include <raycade/physics/PhysicsBody.hpp>
#include <raycade/physics/RigidBody.hpp>
// IWYU pragma: end_exports
