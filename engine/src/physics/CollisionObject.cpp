/**
 * @file CollisionObject.cpp
 * @brief Collidery obiektu, grawitacja sceny i przesuwanie w układzie sceny.
 */
#include <raycade/physics/CollisionObject.hpp>

#include <raycade/physics/PhysicsWorld.hpp>

std::vector<const CollisionShape*> CollisionObject::Shapes() const {
    std::vector<const CollisionShape*> shapes;
    PhysicsWorld::CollectShapes(*this, shapes);
    return shapes;
}

Vector2 CollisionObject::Gravity() const {
    if (const PhysicsWorld* world = Physics()) return world->Gravity();
    return {0.0f, 980.0f};
}

void CollisionObject::MoveInScene(Vector2 delta) {
    const Vector2 position = WorldPosition();
    const Vector2 target{position.x + delta.x, position.y + delta.y};
    if (const Entity* parent = Parent()) {
        if (const auto local = parent->TryToLocal(target)) Position(*local);
    } else {
        Position(target);
    }
}
