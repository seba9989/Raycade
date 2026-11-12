/**
 * @file Updatable.cpp
 * @brief Wywoływanie zdarzeń obiektu: najechanie, kliknięcie, przeciąganie, klawisze.
 */
#include <raycade/entity/Updatable.hpp>

void Updatable::HandleEvents(float dt, const Input& input, const PointerEvents& pointer) {
    if (pointer.hovered && !hovered_ && mouseEnter_) mouseEnter_();
    if (!pointer.hovered && hovered_ && mouseLeave_) mouseLeave_();
    hovered_ = pointer.hovered;
    if (pointer.pressed && click_) click_();
    if (pointer.dragStart && !dragging_) {
        dragging_ = true;
        dragFrom_ = input.MousePosition();
        if (dragStart_) dragStart_();
    }
    if (pointer.released && mouseReleased_) mouseReleased_();
    if (pointer.scrolled && scroll_) scroll_(input.MouseWheel());
    HandleDrag(input);
    for (const auto& [key, callback] : keys_)
        if (input.KeyPressed(key)) callback();
    for (const auto& [key, callback] : keysReleased_)
        if (input.KeyReleased(key)) callback();
    Tick(dt, input);
    if (update_) update_(dt);
}

void Updatable::HandleDrag(const Input& input) {
    if (!dragging_) return;
    // koniec także po wyłączeniu myszy - inaczej obiekt zostałby "przyklejony" do kursora
    if (!input.MouseDown(MOUSE_BUTTON_LEFT)) {
        dragging_ = false;
        if (dragEnd_) dragEnd_();
        return;
    }
    const Vector2 mouse = input.MousePosition();
    if (mouse.x == dragFrom_.x && mouse.y == dragFrom_.y) return;
    const Vector2 delta = DragDelta(dragFrom_, mouse);
    dragFrom_ = mouse;
    if (drag_) drag_(delta);
}
