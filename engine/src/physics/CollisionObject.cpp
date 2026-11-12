/**
 * @file CollisionObject.cpp
 * @brief Collidery obiektu, grawitacja sceny i przesuwanie w układzie sceny.
 */
#include <raycade/physics/CollisionObject.hpp>

#include <raycade/physics/PhysicsWorld.hpp>

#include <cmath>

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

AffineMatrix2D CollisionObject::DrawTransform() const {
    const PhysicsWorld* world = Physics();
    if (!world || !world->IsInterpolation() || !interpolated_) return transform_;
    // przesunięty poza krokiem stałym - rysowany tam, gdzie jest
    const AffineMatrix2D& now = transform_;
    if (now.a != stepped_.a || now.b != stepped_.b || now.c != stepped_.c || now.d != stepped_.d ||
        now.tx != stepped_.tx || now.ty != stepped_.ty)
        return transform_;
    // teleport (np. odrodzenie) - bez smugi przez całą drogę
    if (std::hypot(stepped_.tx - previous_.tx, stepped_.ty - previous_.ty) > 64.0f)
        return transform_;
    const float t = world->Alpha();
    const auto lerp = [t](float from, float to) { return from + (to - from) * t; };
    return {lerp(previous_.a, stepped_.a),   lerp(previous_.b, stepped_.b),
            lerp(previous_.c, stepped_.c),   lerp(previous_.d, stepped_.d),
            lerp(previous_.tx, stepped_.tx), lerp(previous_.ty, stepped_.ty)};
}
