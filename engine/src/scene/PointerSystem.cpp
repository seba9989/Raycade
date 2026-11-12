/**
 * @file PointerSystem.cpp
 * @brief Trafienia myszą: obiekty pod kursorem w kolejności rysowania i odbiorcy zdarzeń.
 */
#include <raycade/scene/PointerSystem.hpp>

#include <algorithm>

PointerFrame PointerSystem::Pick(
    const std::vector<std::unique_ptr<Entity>>& roots,
    const Input& input,
    const SceneCamera& camera
) const {
    PointerFrame frame;
    if (!input.IsMouseEnabled()) return frame;
    frame.pressed = input.MousePressed(MOUSE_BUTTON_LEFT); // także w puste miejsce
    const std::vector<const Entity*> hits = HitsAt(roots, input.MousePosition(), camera);
    if (hits.empty()) return frame;

    frame.target = hits.front();
    if (input.MousePressed(MOUSE_BUTTON_LEFT)) {
        // dynamic_cast tylko przy kliknięciu: pierwszy obiekt z cechą Focusable w górę drzewa
        for (const Entity* hit : hits) {
            for (const Entity* e = hit; e && !frame.focus; e = e->parent_)
                frame.focus = dynamic_cast<const Focusable*>(e);
            if (frame.focus) break;
        }
        frame.press = Receiver(hits, [](const Entity& e) { return e.HandlesClick(); });
        frame.drag = Receiver(hits, [](const Entity& e) { return e.HandlesDrag(); });
    }
    if (input.MouseReleased(MOUSE_BUTTON_LEFT))
        frame.release = Receiver(hits, [](const Entity& e) { return e.HandlesMouseReleased(); });
    if (input.MouseWheel() != 0.0f)
        frame.scroll = Receiver(hits, [](const Entity& e) { return e.HandlesScroll(); });
    return frame;
}

std::vector<const Entity*> PointerSystem::HitsAt(
    const std::vector<std::unique_ptr<Entity>>& roots,
    Vector2 screen,
    const SceneCamera& camera
) const {
    std::vector<const Entity*> hits;
    const Vector2 world = camera.ScreenToWorld(screen);
    const std::vector<const Entity*> order = TopFirst(roots);
    for (const Entity* root : order)
        if (root->screenSpace_) CollectHits(*root, screen, hits);
    for (const Entity* root : order)
        if (!root->screenSpace_) CollectHits(*root, world, hits);
    return hits;
}

void PointerSystem::CollectHits(
    const Entity& entity,
    Vector2 point,
    std::vector<const Entity*>& hits
) {
    if (entity.destroyed_) return;
    // poza przycięciem nie widać ani obiektu, ani dzieci - nie da się ich kliknąć
    if (const auto& clip = entity.ClipRect()) {
        const auto local = entity.TryToLocal(point);
        if (!local || !::CheckCollisionPointRec(*local, *clip)) return;
    }
    if (entity.ChildrenHittable(point))
        for (const Entity* child : TopFirst(entity.children_))
            CollectHits(*child, point, hits);
    if (entity.Contains(point)) hits.push_back(&entity);
}

std::vector<const Entity*> PointerSystem::TopFirst(
    const std::vector<std::unique_ptr<Entity>>& entities
) {
    std::vector<const Entity*> order;
    order.reserve(entities.size());
    for (const auto& entity : entities)
        order.push_back(entity.get());
    std::stable_sort(order.begin(), order.end(), [](const Entity* a, const Entity* b) {
        return a->Layer() > b->Layer();
    });
    // w tej samej warstwie później dodany jest rysowany wyżej
    for (auto begin = order.begin(); begin != order.end();) {
        auto end = std::find_if(begin, order.end(), [&](const Entity* e) {
            return e->Layer() != (*begin)->Layer();
        });
        std::reverse(begin, end);
        begin = end;
    }
    return order;
}
