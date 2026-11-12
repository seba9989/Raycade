/**
 * @file CharacterBody.cpp
 * @brief Ruch postaci przez świat fizyki sceny.
 */
#include <raycade/physics/CharacterBody.hpp>

#include <raycade/physics/PhysicsWorld.hpp>

bool CharacterBody::MoveAndSlide() {
    if (PhysicsWorld* world = Physics()) return world->MoveAndSlide(*this);
    // obiekt spoza sceny nie ma z czym się zderzyć ani znanego kroku czasu
    collisions_.clear();
    onFloor_ = onWall_ = onCeiling_ = false;
    return false;
}
