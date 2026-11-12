/**
 * @file RigidBody.cpp
 * @brief Kąt obrotu bryły (pozostałe obliczenia są w PhysicsWorld).
 */
#include <raycade/physics/RigidBody.hpp>

#include <cmath>

float RigidBody::Rotation() const {
    const AffineMatrix2D world = WorldTransform();
    return std::atan2(world.b, world.a);
}
