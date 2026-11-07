/**
 * @file SceneCamera.cpp
 * @brief Płynne podążanie kamery za celem.
 */
#include <raycade/scene/SceneCamera.hpp>

void SceneCamera::Update(float dt) {
    if (!follow_) return;
    const Vector2 goal = follow_();
    if (followSpeed_ <= 0.0f) {
        camera_.target = goal;
        return;
    }
    // wygładzanie wykładnicze - niezależne od liczby klatek na sekundę
    const float t = 1.0f - std::exp(-followSpeed_ * dt);
    camera_.target.x += (goal.x - camera_.target.x) * t;
    camera_.target.y += (goal.y - camera_.target.y) * t;
}
