/**
 * @file CollisionShape.cpp
 * @brief Wyszukiwanie obiektu kolizji, do którego należy collider.
 */
#include <raycade/node/collider/CollisionShape.hpp>

#include <raycade/node/physics/CollisionObject.hpp>

namespace node::collider {

physics::CollisionObject* CollisionShape::Object() const {
    for (Entity* entity = Node().Parent(); entity; entity = entity->Parent())
        if (auto* object = dynamic_cast<physics::CollisionObject*>(entity)) return object;
    return nullptr;
}

} // namespace node::collider
