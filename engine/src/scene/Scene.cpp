/**
 * @file Scene.cpp
 * @brief Klatka sceny: aktualizacja obiektów, fokus, rysowanie świata i ekranu.
 */
#include <raycade/scene/Scene.hpp>

void Scene::Update(float dt, const Input& input) {
    if (update_) update_(dt);
    // pętla po indeksie: zdarzenie obiektu może wywołać Spawn() i powiększyć wektor
    // Tab działa także przy przejętej klawiaturze - inaczej z pola tekstowego nie dałoby
    // się przejść dalej
    if (tabNavigation_ && input.SystemKeyPressed(KEY_TAB))
        MoveFocus(input.SystemKeyDown(KEY_LEFT_SHIFT) || input.SystemKeyDown(KEY_RIGHT_SHIFT) ? -1
                                                                                              : 1);

    // trafienia myszą liczone raz, przed zmianami w tej klatce (patrz Entity::Pointer)
    const PointerFrame pointer = Entity::Pointer(entities_, input, camera_);
    // kliknięcie daje fokus trafionemu obiektowi z cechą Focusable, a w cokolwiek innego -
    // zabiera
    if (pointer.pressed) focus_.focused = pointer.focus;
    NotifyFocus(); // także zmiany z Focus()/Blur() wywołanych w poprzedniej klatce

    PointerFrame frame = pointer;
    frame.focused = focus_.focused ? focus_.focused->Owner() : nullptr;
    for (std::size_t i = 0; i < entities_.size(); ++i)
        entities_[i]->Update(dt, input, frame);
    // po ruchu obiektów, żeby kamera w tej samej klatce pokazała ich nowe położenie
    camera_.Update(dt);
}

void Scene::Draw(PrimitiveRenderer& renderer) const {
    ::BeginMode2D(camera_.Raw());
    if (drawWorld_) drawWorld_(renderer.Reset().TargetScreen());
    DrawRoots(renderer, false);
    ::EndMode2D();
    DrawRoots(renderer, true);
    if (draw_) draw_(renderer.Reset().TargetScreen());
}

void Scene::NotifyFocus() {
    if (focus_.focused == focus_.notified) return;
    if (focus_.notified && focus_.notified->blur_) focus_.notified->blur_();
    focus_.notified = focus_.focused;
    if (focus_.notified && focus_.notified->focus_) focus_.notified->focus_();
}

void Scene::MoveFocus(int step) {
    const std::vector<const Focusable*> order = Focusable::TabOrder(focus_);
    if (order.empty()) return;
    const auto current = std::find(order.begin(), order.end(), focus_.focused);
    const int size = static_cast<int>(order.size());
    int index =
        current == order.end() ? (step > 0 ? -1 : size) : static_cast<int>(current - order.begin());
    index = ((index + step) % size + size) % size;
    focus_.focused = order[static_cast<std::size_t>(index)];
}

void Scene::DrawRoots(PrimitiveRenderer& renderer, bool screenSpace) const {
    std::vector<const Drawable*> order;
    order.reserve(entities_.size());
    for (const auto& entity : entities_)
        if (entity->IsScreenSpace() == screenSpace) order.push_back(entity.get());
    for (const Drawable* entity : Drawable::ByLayer(std::move(order)))
        entity->Render(renderer);
}
