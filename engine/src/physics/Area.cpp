/**
 * @file Area.cpp
 * @brief Zapytania o obiekty nachodzące na obszar.
 */
#include <raycade/physics/Area.hpp>

#include <raycade/physics/PhysicsWorld.hpp>

std::vector<PhysicsBody*> Area::OverlappingBodies() const {
    if (PhysicsWorld* world = Physics()) return world->OverlappingBodies(*this);
    return {};
}

std::vector<Area*> Area::OverlappingAreas() const {
    if (PhysicsWorld* world = Physics()) return world->OverlappingAreas(*this);
    return {};
}
