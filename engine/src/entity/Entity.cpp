/**
 * @file Entity.cpp
 * @brief Drzewo obiektów: rysowanie dzieci, współrzędne, zdarzenia myszy i podpinanie do sceny.
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

void Entity::FixedUpdate(float step, const Input& input) {
    if (destroyed_) return;
    HandleFixed(step, input);
    // po indeksie: callback może dodać dziecko
    for (std::size_t i = 0; i < children_.size(); ++i)
        children_[i]->FixedUpdate(step, input);
}

void Entity::Render(PrimitiveRenderer& renderer) const {
    const auto matrix = DrawTransform().RlglMatrix();
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

void Entity::Attach(const SceneLinks& links) {
    links_ = &links;
    // dynamic_cast raz przy dodaniu do sceny, a nie co klatkę (drugi jest
    // w PointerSystem::Pick(), tylko w klatce kliknięcia)
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
