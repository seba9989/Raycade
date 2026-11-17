/**
 * @file StaticBody.hpp
 * @brief Klasa StaticBody - nieruchome ciało.
 */
#pragma once

#include <raycade/node/physics/PhysicsBody.hpp>

namespace node::physics {

/**
 * @brief Nieruchome ciało: ściana, podłoga, platforma (StaticBody2D z Godota).
 * @details Inne ciała odbijają się od niego, a ono samo nie jest przesuwane przy zderzeniach.
 * Można je przesuwać w kodzie (Position(), Translate()) - np. ruchoma platforma.
 */
class StaticBody : public PhysicsBody {};

} // namespace node::physics
