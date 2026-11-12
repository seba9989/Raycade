/**
 * @file RenderGroup.cpp
 * @brief Rysowanie dzieci do tekstury GPU i wyświetlanie jej.
 */
#include <raycade/visual/RenderGroup.hpp>

namespace visual {

bool RenderGroup::Contains(Vector2 point) const {
    const auto local = TryToLocal(point);
    return local && ::CheckCollisionPointRec(*local, {0, 0, size_.x, size_.y});
}

void RenderGroup::RenderContent(PrimitiveRenderer& renderer) const {
    if (size_.x < 1 || size_.y < 1) return;
    if (!target_.IsValid()) {
        target_ = RenderTarget{static_cast<int>(size_.x), static_cast<int>(size_.y)};
        target_.Smooth(smooth_);
    }
    target_.Begin();
    target_.Clear(background_);
    RenderChildren(renderer);
    target_.End();
    // tryb mieszania i shader działają na gotowej teksturze - efekt obejmuje całą grupę dzieci
    BeginEffects();
    target_.Draw(Vector2{0, 0}, color_);
    EndEffects();
}

} // namespace visual
