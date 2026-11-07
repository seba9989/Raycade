/**
 * @file Focusable.cpp
 * @brief Rejestracja obiektów z fokusem i kolejność przechodzenia Tabem.
 */
#include <raycade/entity/Focusable.hpp>

Focusable::~Focusable() {
    if (!state_) return;
    std::erase(state_->members, this);
    if (state_->focused == this) state_->focused = nullptr;
    if (state_->notified == this) state_->notified = nullptr; // bez OnBlur() na martwym
}

void Focusable::AttachFocus(FocusState& state, const Entity* owner) {
    if (state_ == &state) return; // już zarejestrowany
    state_ = &state;
    owner_ = owner;
    state.members.push_back(this);
    if (wantsFocus_) {
        wantsFocus_ = false;
        state.focused = this;
    }
}

std::vector<const Focusable*> Focusable::TabOrder(const FocusState& state) {
    std::vector<const Focusable*> order;
    for (const Focusable* f : state.members)
        if (f->tabIndex_ >= 0) order.push_back(f);
    std::ranges::stable_sort(order, {}, [](const Focusable* f) {
        return f->tabIndex_ == 0 ? INT_MAX : f->tabIndex_;
    });
    return order;
}
