/**
 * @file CollisionShape.cpp
 * @brief Wyszukiwanie obiektu kolizji, do którego należy collider.
 */
#include <raycade/physics/CollisionShape.hpp>

#include <raycade/physics/CollisionObject.hpp>

CollisionObject* CollisionShape::Object() const {
    for (Entity* entity = Node().Parent(); entity; entity = entity->Parent())
        if (auto* object = dynamic_cast<CollisionObject*>(entity)) return object;
    return nullptr;
}
