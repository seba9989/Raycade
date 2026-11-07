/**
 * @file Entity.cpp
 * @brief Drzewo obiektów: rysowanie dzieci, testy trafień, rozdział zdarzeń myszy i podpinanie do
 * sceny.
 */
#include <raycade/entity/Entity.hpp>

#include <raycade/render/ClipStack.hpp>
#include <raycade/scene/SceneCamera.hpp>

void Entity::Update(float dt, const Input& input, const PointerFrame& pointer) {
    if (destroyed_) return; // czeka na usunięcie przez scenę na końcu klatki
    PointerEvents events;
    const bool focused = pointer.focused == this;
    events.hovered = IsSelfOrAncestorOf(pointer.target);
    events.pressed = pointer.press == this;
    events.released = pointer.release == this;
    events.scrolled = pointer.scroll == this;
    events.dragStart = pointer.drag == this;
    if (focused && input.IsKeyboardCaptured()) {
        // obiekt z fokusem jako jedyny widzi przejętą przez siebie klawiaturę
        HandleEvents(dt, input.Uncaptured(), events);
    } else {
        HandleEvents(dt, input, events);
    }
    // Callback może dodać dziecko, dlatego rozmiar sprawdzamy w każdej iteracji.
    for (std::size_t i = 0; i < children_.size(); ++i)
        children_[i]->Update(dt, input, pointer);
}

void Entity::Render(PrimitiveRenderer& renderer) const {
    const auto matrix = transform_.RlglMatrix();
    rlPushMatrix();
    rlMultMatrixf(matrix.data());
    // przycięcie obejmuje własny rysunek i dzieci (prostokąt w układzie lokalnym)
    if (clip_) ClipStack::Push(*clip_);
    RenderContent(renderer);
    if (clip_) ClipStack::Pop();
    rlPopMatrix();
}

bool Entity::IsScreenSpace() const {
    const Entity* root = this;
    while (root->parent_)
        root = root->parent_;
    return root->screenSpace_;
}

const Contextable* Entity::ContextParent() const {
    if (parent_) return parent_;
    if (links_) return links_->context;
    return ContextRoot();
}

Vector2 Entity::DragDelta(Vector2 screenFrom, Vector2 screenTo) const {
    const Vector2 from = ScreenToScene(screenFrom);
    const Vector2 to = ScreenToScene(screenTo);
    if (!parent_) return Updatable::DragDelta(from, to);
    const auto a = parent_->TryToLocal(from);
    const auto b = parent_->TryToLocal(to);
    if (!a || !b) return Updatable::DragDelta(from, to);
    return {b->x - a->x, b->y - a->y};
}

void Entity::RenderChildren(PrimitiveRenderer& renderer) const {
    std::vector<const Drawable*> order;
    order.reserve(children_.size());
    for (const auto& child : children_)
        order.push_back(child.get());
    for (const Drawable* child : Drawable::ByLayer(std::move(order)))
        child->Render(renderer);
}

std::vector<Vector2> Entity::WorldVertices(std::span<const Point2D> local) const {
    std::vector<Vector2> world;
    world.reserve(local.size());
    for (const Point2D vertex : local)
        world.push_back(World(vertex));
    return world;
}

bool Entity::AnyChildContains(Vector2 point) const {
    for (const auto& child : children_)
        if (child->Contains(point) || child->AnyChildContains(point)) return true;
    return false;
}

void Entity::CollectHits(Vector2 point, std::vector<const Entity*>& hits) const {
    if (destroyed_) return;
    // poza przycięciem nie widać ani obiektu, ani dzieci - nie da się ich kliknąć
    if (clip_) {
        const auto local = TryToLocal(point);
        if (!local || !::CheckCollisionPointRec(*local, *clip_)) return;
    }
    if (ChildrenHittable(point))
        for (const Entity* child : TopFirst(children_))
            child->CollectHits(point, hits);
    if (Contains(point)) hits.push_back(this);
}

std::vector<const Entity*> Entity::TopFirst(const std::vector<std::unique_ptr<Entity>>& entities) {
    std::vector<const Entity*> order;
    order.reserve(entities.size());
    for (const auto& entity : entities)
        order.push_back(entity.get());
    std::stable_sort(order.begin(), order.end(),
                     [](const Entity* a, const Entity* b) { return a->Layer() > b->Layer(); });
    // w tej samej warstwie później dodany jest rysowany wyżej
    for (auto begin = order.begin(); begin != order.end();) {
        auto end = std::find_if(begin, order.end(),
                                [&](const Entity* e) { return e->Layer() != (*begin)->Layer(); });
        std::reverse(begin, end);
        begin = end;
    }
    return order;
}

void Entity::Attach(const SceneLinks& links) {
    links_ = &links;
    // dynamic_cast raz przy dodaniu do sceny, a nie co klatkę (drugi jest w Pointer(),
    // tylko w klatce kliknięcia)
    if (Focusable* focusable = dynamic_cast<Focusable*>(this))
        focusable->AttachFocus(*links.focus, this);
    if (!mounted_) {
        mounted_ = true;
        OnMount();
    }
    // po indeksie: OnMount() może dodać dzieci (Spawn podepnie je od razu)
    for (std::size_t i = 0; i < children_.size(); ++i)
        if (!children_[i]->mounted_) children_[i]->Attach(links);
}

Vector2 Entity::ScreenPosition() const {
    const SceneCamera* camera = ViewCamera();
    return camera ? camera->WorldToScreen(WorldPosition()) : WorldPosition();
}

PointerFrame Entity::Pointer(const std::vector<std::unique_ptr<Entity>>& roots, const Input& input,
                             const SceneCamera& camera) {
    PointerFrame frame;
    if (!input.IsMouseEnabled()) return frame;
    frame.pressed = input.MousePressed(MOUSE_BUTTON_LEFT); // także w puste miejsce
    std::vector<const Entity*> hits;
    const Vector2 mouse = input.MousePosition();
    const Vector2 worldMouse = camera.ScreenToWorld(mouse);
    const std::vector<const Entity*> order = TopFirst(roots);
    for (const Entity* root : order)
        if (root->screenSpace_) root->CollectHits(mouse, hits);
    for (const Entity* root : order)
        if (!root->screenSpace_) root->CollectHits(worldMouse, hits);
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
        frame.release = Receiver(hits, [](const Entity& e) { return e.HandlesMouseUp(); });
    if (input.MouseWheel() != 0.0f)
        frame.scroll = Receiver(hits, [](const Entity& e) { return e.HandlesScroll(); });
    return frame;
}

const SceneCamera* Entity::ViewCamera() const {
    return IsScreenSpace() || !links_ ? nullptr : links_->camera;
}

Vector2 Entity::ScreenToScene(Vector2 point) const {
    const SceneCamera* camera = ViewCamera();
    return camera ? camera->ScreenToWorld(point) : point;
}

bool Entity::IsSelfOrAncestorOf(const Entity* entity) const {
    for (; entity; entity = entity->parent_)
        if (entity == this) return true;
    return false;
}
