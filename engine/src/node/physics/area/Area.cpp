/**
 * @file Area.cpp
 * @brief Zapytania o obiekty nachodzące na obszar.
 */
#include <raycade/node/physics/area/Area.hpp>

#include <raycade/scene/physics/PhysicsWorld.hpp>

namespace node::physics {

std::vector<PhysicsBody*> Area::OverlappingBodies() const {
    if (PhysicsWorld* world = Physics()) return world->OverlappingBodies(*this);
    return {};
}

std::vector<Area*> Area::OverlappingAreas() const {
    if (PhysicsWorld* world = Physics()) return world->OverlappingAreas(*this);
    return {};
}

} // namespace node::physics
