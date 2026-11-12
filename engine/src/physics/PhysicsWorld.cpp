/**
 * @file PhysicsWorld.cpp
 * @brief Zbieranie ciał, MoveAndSlide(), obszary, zdarzenia zderzeń i podgląd colliderów.
 */
#include <raycade/physics/PhysicsWorld.hpp>

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

float Dot(Vector2 a, Vector2 b) {
    return a.x * b.x + a.y * b.y;
}

/** @brief Najkrótszy odcinek ruchu w jednym podkroku MoveAndSlide() (przeciw tunelowaniu). */
constexpr float kMaxSubstepDistance = 8.0f;
constexpr int kMaxSubsteps = 16;
constexpr int kMaxPushes = 4;
constexpr float kMinDepth = 1e-4f;
/** @brief Zapas wnikania w platformę jednostronną, przy którym nadal zatrzymuje postać. */
constexpr float kOneWayMargin = 1.0f;
/** @brief Najmniejszy cosinus kąta między wypchnięciem a „górą” platformy jednostronnej. */
constexpr float kOneWayCos = 0.7f;

/** @brief Zdarzenie do wysłania po zakończeniu analizy kroku (bez zmian list w trakcie). */
using Event = std::function<void()>;

} // namespace

void PhysicsWorld::BeginFrame(float dt) {
    delta_ = dt;
    dirty_ = true;
}

void PhysicsWorld::BeginStep(float step) {
    delta_ = step;
    dirty_ = true;
}

void PhysicsWorld::CollectShapes(const Entity& object, std::vector<const CollisionShape*>& shapes) {
    for (const auto& child : object.children_) {
        if (child->destroyed_) continue;
        // potomkowie innego obiektu kolizji należą do niego
        if (dynamic_cast<const CollisionObject*>(child.get())) continue;
        if (const auto* shape = dynamic_cast<const CollisionShape*>(child.get()))
            shapes.push_back(shape);
        CollectShapes(*child, shapes);
    }
}

const std::vector<PhysicsWorld::Entry>& PhysicsWorld::Entries() const {
    if (!dirty_) return entries_;
    entries_.clear();
    for (const auto& root : roots_)
        if (!root->screenSpace_) Collect(*root, -1);
    dirty_ = false;
    return entries_;
}

void PhysicsWorld::Collect(const Entity& entity, int owner) const {
    if (entity.destroyed_) return;
    // const_cast: świat dostaje drzewo sceny tylko do odczytu, ale wywołuje zdarzenia obiektów
    auto* object = dynamic_cast<CollisionObject*>(const_cast<Entity*>(&entity));
    if (object) {
        Entry entry;
        entry.object = object;
        entry.body = dynamic_cast<PhysicsBody*>(object);
        entry.area = dynamic_cast<Area*>(object);
        entries_.push_back(std::move(entry));
        owner = static_cast<int>(entries_.size()) - 1;
    } else if (const auto* shape = dynamic_cast<const CollisionShape*>(&entity);
               shape && owner >= 0) {
        entries_[static_cast<std::size_t>(owner)].shapes.push_back(shape);
    }
    for (const auto& child : entity.children_)
        Collect(*child, owner);
}

const PhysicsWorld::Entry* PhysicsWorld::Find(const CollisionObject* object) const {
    for (const Entry& entry : Entries())
        if (entry.object == object) return &entry;
    return nullptr;
}

std::optional<Collision> PhysicsWorld::Deepest(const Entry& self, Vector2 motion) const {
    std::optional<Collision> deepest;
    std::vector<ConvexPart> mine;
    std::vector<ConvexPart> theirs;
    for (const Entry& other : Entries()) {
        if (other.object == self.object || !other.body || other.object->IsDestroyed()) continue;
        if (!self.object->CollidesWith(*other.object)) continue;
        for (const CollisionShape* shape : self.shapes) {
            if (shape->IsDisabled()) continue;
            mine.clear();
            shape->WorldParts(mine);
            for (const CollisionShape* otherShape : other.shapes) {
                if (otherShape->IsDisabled()) continue;
                theirs.clear();
                otherShape->WorldParts(theirs);
                for (const ConvexPart& a : mine)
                    for (const ConvexPart& b : theirs) {
                        const auto hit = ConvexPenetration(a, b);
                        if (!hit || hit->depth < kMinDepth) continue;
                        if (otherShape->IsOneWay()) {
                            // tylko opadanie na platformę od góry: wypchnięcie w jej „górę”,
                            // ruch nie w górę i wnikanie nie głębsze niż ten ruch
                            const Vector2 up = otherShape->OneWayDirection();
                            const float down = -Dot(motion, up);
                            if (Dot(hit->normal, up) < kOneWayCos || down < 0.0f ||
                                hit->depth > down + kOneWayMargin)
                                continue;
                        }
                        if (deepest && deepest->depth >= hit->depth) continue;
                        deepest = Collision{other.body, shape, otherShape, hit->normal, hit->depth};
                    }
            }
        }
    }
    return deepest;
}

bool PhysicsWorld::Overlap(const Entry& a, const Entry& b) {
    std::vector<ConvexPart> partsA;
    std::vector<ConvexPart> partsB;
    for (const CollisionShape* shape : a.shapes)
        if (!shape->IsDisabled()) shape->WorldParts(partsA);
    for (const CollisionShape* shape : b.shapes)
        if (!shape->IsDisabled()) shape->WorldParts(partsB);
    for (const ConvexPart& partA : partsA)
        for (const ConvexPart& partB : partsB)
            if (ConvexOverlap(partA, partB)) return true;
    return false;
}

void PhysicsWorld::Record(CharacterBody& body, const Collision& hit) {
    const bool bodyFirst = std::less<const CollisionObject*>{}(&body, hit.other);
    const Pair key = bodyFirst ? Pair{&body, hit.other} : Pair{hit.other, &body};
    if (contacts_.contains(key)) return; // w kroku liczy się pierwsze zderzenie pary
    const Collision mirrored{
        &body, hit.otherShape, hit.shape, {-hit.normal.x, -hit.normal.y}, hit.depth
    };
    Contact contact;
    contact.first = bodyFirst ? &body : hit.other;
    contact.second = bodyFirst ? hit.other : &body;
    contact.forFirst = bodyFirst ? hit : mirrored;
    contact.forSecond = bodyFirst ? mirrored : hit;
    contacts_.emplace(key, contact);
}

bool PhysicsWorld::MoveAndSlide(CharacterBody& body) {
    const bool wasOnFloor = body.onFloor_;
    body.collisions_.clear();
    body.onFloor_ = body.onWall_ = body.onCeiling_ = false;

    const Entry* self = Find(&body);
    const Vector2 motion{body.velocity_.x * delta_, body.velocity_.y * delta_};
    if (!self || self->shapes.empty()) {
        body.MoveInScene(motion); // bez colliderów - zwykły ruch
        return false;
    }

    // wożenie przez platformę: przesunięcie punktu pod postacią od jej ostatniego ruchu
    body.platformVelocity_ = {};
    const PhysicsBody* floorBody = wasOnFloor ? body.floorBody_ : nullptr;
    // postać przeniesiona w kodzie (np. Position() przy odrodzeniu) nie stoi już na tej podłodze
    const Vector2 position = body.WorldPosition();
    const bool moved = std::fabs(position.x - body.floorWorld_.x) > 0.01f ||
                       std::fabs(position.y - body.floorWorld_.y) > 0.01f;
    if (floorBody && !moved && Find(floorBody)) {
        const Vector2 now = floorBody->World(body.floorLocal_);
        const Vector2 carry{now.x - body.floorWorld_.x, now.y - body.floorWorld_.y};
        if (Dot(carry, carry) > 0.0f) {
            body.MoveInScene(carry);
            if (delta_ > 0.0f) body.platformVelocity_ = {carry.x / delta_, carry.y / delta_};
        }
    }
    const PhysicsBody* standingOn = nullptr; // podłoga po tym ruchu

    const float distance = std::sqrt(Dot(motion, motion));
    const int substeps =
        std::clamp(static_cast<int>(std::ceil(distance / kMaxSubstepDistance)), 1, kMaxSubsteps);
    const float substepTime = delta_ / static_cast<float>(substeps);
    const Vector2 up = body.up_;
    const float floorCos = std::cos(body.floorMaxAngle_);

    // rozsunięcie z jednym ciałem: podłoga - pionowo w górę, ściana i sufit - wzdłuż normalnej
    const auto resolve = [&](const Collision& hit) {
        const Vector2 n = hit.normal;
        const float upness = Dot(n, up);
        if (upness >= floorCos) {
            // podłoga: wypchnięcie pionowo w górę (bez zsuwania się z pochylni)
            body.onFloor_ = true;
            body.floorNormal_ = n;
            standingOn = hit.other;
            const float lift = hit.depth / upness;
            body.MoveInScene({up.x * lift, up.y * lift});
            const float into = Dot(body.velocity_, up);
            if (into < 0.0f)
                body.velocity_ = {body.velocity_.x - up.x * into, body.velocity_.y - up.y * into};
        } else {
            if (upness <= -floorCos)
                body.onCeiling_ = true;
            else
                body.onWall_ = true;
            body.MoveInScene({n.x * hit.depth, n.y * hit.depth});
            const float into = Dot(body.velocity_, n);
            if (into < 0.0f)
                body.velocity_ = {body.velocity_.x - n.x * into, body.velocity_.y - n.y * into};
        }
        const bool known =
            std::any_of(body.collisions_.begin(), body.collisions_.end(), [&](const Collision& c) {
                return c.other == hit.other && c.shape == hit.shape;
            });
        if (!known) body.collisions_.push_back(hit);
        Record(body, hit);
    };

    for (int substep = 0; substep < substeps; ++substep) {
        const Vector2 step{body.velocity_.x * substepTime, body.velocity_.y * substepTime};
        body.MoveInScene(step);
        for (int push = 0; push < kMaxPushes; ++push) {
            const std::optional<Collision> hit = Deepest(*self, step);
            if (!hit) break;
            resolve(*hit);
        }
    }

    // Przyklejenie do podłogi (floor snap): schodząc z pochylni, postać w każdym kroku przesuwa
    // się w bok szybciej, niż grawitacja ściąga ją w dół, i odrywa się od powierzchni. Jeśli
    // przed ruchem stała na podłodze i nie leci w górę (skok), sprawdzamy podłogę tuż pod nią.
    const float snap = body.floorSnapLength_;
    if (wasOnFloor && !body.onFloor_ && snap > 0.0f && Dot(body.velocity_, up) <= 0.0f) {
        const Vector2 before = body.Position();
        body.MoveInScene({-up.x * snap, -up.y * snap});
        bool floor = false;
        for (int push = 0; push < kMaxPushes; ++push) {
            const std::optional<Collision> hit = Deepest(*self, {-up.x * snap, -up.y * snap});
            if (!hit) break;
            if (Dot(hit->normal, up) < floorCos) { // pod spodem nie ma podłogi - np. ściana
                floor = false;
                break;
            }
            resolve(*hit);
            floor = true;
        }
        if (!floor) body.Position(before); // nic pod spodem - postać spada normalnie
    }

    // zapamiętanie podłogi do wożenia w następnym ruchu
    body.floorBody_ = body.onFloor_ ? standingOn : nullptr;
    if (body.floorBody_) {
        body.floorWorld_ = body.WorldPosition();
        body.floorLocal_ = body.floorBody_->ToLocal(body.floorWorld_);
    }
    // zejście z jadącej platformy (skok, krawędź) - postać zachowuje jej prędkość
    if (wasOnFloor && !body.onFloor_)
        body.velocity_ = {
            body.velocity_.x + body.platformVelocity_.x, body.velocity_.y + body.platformVelocity_.y
        };
    return !body.collisions_.empty();
}

std::vector<PhysicsBody*> PhysicsWorld::OverlappingBodies(const Area& area) {
    std::vector<PhysicsBody*> bodies;
    const Entry* self = Find(&area);
    if (!self) return bodies;
    for (const Entry& other : Entries())
        if (other.body && other.object != &area && !other.object->IsDestroyed() &&
            area.CollidesWith(*other.object) && Overlap(*self, other))
            bodies.push_back(other.body);
    return bodies;
}

std::vector<Area*> PhysicsWorld::OverlappingAreas(const Area& area) {
    std::vector<Area*> areas;
    const Entry* self = Find(&area);
    if (!self) return areas;
    for (const Entry& other : Entries())
        if (other.area && other.object != &area && !other.object->IsDestroyed() &&
            area.CollidesWith(*other.object) && Overlap(*self, other))
            areas.push_back(other.area);
    return areas;
}

std::optional<RayHit> PhysicsWorld::Raycast(
    Vector2 from,
    Vector2 to,
    std::uint32_t mask,
    const CollisionObject* exclude
) const {
    std::optional<RayHit> nearest;
    float nearestFraction = INFINITY;
    const Vector2 direction{to.x - from.x, to.y - from.y};
    const float length = std::sqrt(Dot(direction, direction));
    std::vector<ConvexPart> parts;
    for (const Entry& entry : Entries()) {
        if (!entry.body || entry.object == exclude || entry.object->IsDestroyed()) continue;
        if ((mask & entry.object->CollisionLayer()) == 0) continue;
        for (const CollisionShape* shape : entry.shapes) {
            if (shape->IsDisabled()) continue;
            // platforma jednostronna zatrzymuje tylko promień lecący z jej „góry”
            if (shape->IsOneWay() && Dot(direction, shape->OneWayDirection()) >= 0.0f) continue;
            parts.clear();
            shape->WorldParts(parts);
            for (const ConvexPart& part : parts) {
                const auto hit = ConvexRaycast(part, from, to);
                if (!hit || hit->fraction >= nearestFraction) continue;
                nearestFraction = hit->fraction;
                nearest = RayHit{
                    entry.body,
                    shape,
                    {from.x + direction.x * hit->fraction, from.y + direction.y * hit->fraction},
                    hit->normal,
                    length * hit->fraction
                };
            }
        }
    }
    return nearest;
}

std::vector<PhysicsBody*> PhysicsWorld::Overlap(
    const ConvexPart& part,
    std::uint32_t mask,
    const CollisionObject* exclude
) const {
    std::vector<PhysicsBody*> bodies;
    std::vector<ConvexPart> parts;
    for (const Entry& entry : Entries()) {
        if (!entry.body || entry.object == exclude || entry.object->IsDestroyed()) continue;
        if ((mask & entry.object->CollisionLayer()) == 0) continue;
        bool hit = false;
        for (const CollisionShape* shape : entry.shapes) {
            if (shape->IsDisabled() || hit) continue;
            parts.clear();
            shape->WorldParts(parts);
            for (const ConvexPart& other : parts)
                if (ConvexOverlap(part, other)) hit = true;
        }
        if (hit) bodies.push_back(entry.body);
    }
    return bodies;
}

void PhysicsWorld::EndStep() {
    const std::vector<Entry>& entries = Entries();
    // obiekty usunięte ze sceny przed tym krokiem nie istnieją - nie wolno ich dotykać
    const auto present = [&](const CollisionObject* object) {
        return std::any_of(entries.begin(), entries.end(), [object](const Entry& e) {
            return e.object == object;
        });
    };

    // najpierw zebranie zdarzeń, potem wywołanie: callback może usuwać i dodawać obiekty
    std::vector<Event> events;

    for (const auto& [key, contact] : contacts_) {
        // zderzenie z MoveAndSlide() w Tick() mogło dotyczyć obiektu usuniętego potem ze sceny
        if (!present(key.first) || !present(key.second)) continue;
        const bool stay = lastContacts_.contains(key);
        PhysicsBody* first = contact.first;
        PhysicsBody* second = contact.second;
        const Collision forFirst = contact.forFirst;
        const Collision forSecond = contact.forSecond;
        events.emplace_back([=] {
            const auto& call = stay ? first->stay_ : first->enter_;
            if (call) call(forFirst);
        });
        events.emplace_back([=] {
            const auto& call = stay ? second->stay_ : second->enter_;
            if (call) call(forSecond);
        });
    }
    for (const auto& [key, contact] : lastContacts_) {
        if (contacts_.contains(key) || !present(key.first) || !present(key.second)) continue;
        PhysicsBody* first = contact.first;
        PhysicsBody* second = contact.second;
        events.emplace_back([=] {
            if (first->exit_) first->exit_(*second);
            if (second->exit_) second->exit_(*first);
        });
    }
    lastContacts_ = std::move(contacts_);
    contacts_.clear();

    // obszary: pary (obszar, wykryty obiekt) i pary colliderów (do OnBodyShapeEnter/Exit)
    std::set<Pair> overlaps;
    std::set<ShapePair> shapeOverlaps;
    std::vector<ConvexPart> areaParts;
    std::vector<ConvexPart> otherParts;
    const auto touching = [&](const CollisionShape& a, const CollisionShape& b) {
        areaParts.clear();
        otherParts.clear();
        a.WorldParts(areaParts);
        b.WorldParts(otherParts);
        for (const ConvexPart& partA : areaParts)
            for (const ConvexPart& partB : otherParts)
                if (ConvexOverlap(partA, partB)) return true;
        return false;
    };
    for (const Entry& area : entries) {
        if (!area.area || area.object->IsDestroyed()) continue;
        Area* self = area.area;
        for (const Entry& other : entries) {
            if (other.object == area.object || other.object->IsDestroyed()) continue;
            if (!area.object->CollidesWith(*other.object)) continue;
            bool any = false;
            for (const CollisionShape* areaShape : area.shapes) {
                if (areaShape->IsDisabled()) continue;
                for (const CollisionShape* otherShape : other.shapes) {
                    if (otherShape->IsDisabled() || !touching(*areaShape, *otherShape)) continue;
                    any = true;
                    if (!other.body) continue;
                    const ShapePair shapeKey{area.object, areaShape, other.object, otherShape};
                    shapeOverlaps.insert(shapeKey);
                    if (lastShapeOverlaps_.contains(shapeKey)) continue;
                    const AreaShapeContact contact{other.body, otherShape, areaShape};
                    events.emplace_back([self, contact] {
                        if (self->shapeEnter_) self->shapeEnter_(contact);
                    });
                }
            }
            if (!any) continue;
            const Pair key{area.object, other.object};
            overlaps.insert(key);
            if (lastOverlaps_.contains(key)) continue;
            if (other.body) {
                PhysicsBody* body = other.body;
                events.emplace_back([self, body] {
                    if (self->bodyEnter_) self->bodyEnter_(*body);
                });
            } else if (other.area) {
                Area* entered = other.area;
                events.emplace_back([self, entered] {
                    if (self->areaEnter_) self->areaEnter_(*entered);
                });
            }
        }
    }
    // obiekt usunięty w tym kroku jeszcze istnieje (Destroy() usuwa go na końcu klatki), ale ten
    // usunięty przed krokiem - już nie: zdarzenie wyjścia tylko dla obiektów z bieżącej listy
    const auto entryOf = [&](const CollisionObject* object) -> const Entry* {
        for (const Entry& entry : entries)
            if (entry.object == object) return &entry;
        return nullptr;
    };
    const auto ownsShape = [](const Entry* entry, const CollisionShape* shape) {
        return entry &&
               std::find(entry->shapes.begin(), entry->shapes.end(), shape) != entry->shapes.end();
    };
    for (const ShapePair& key : lastShapeOverlaps_) {
        if (shapeOverlaps.contains(key)) continue;
        const auto& [areaObject, areaShape, bodyObject, bodyShape] = key;
        const Entry* area = entryOf(areaObject);
        const Entry* other = entryOf(bodyObject);
        if (!ownsShape(area, areaShape) || !ownsShape(other, bodyShape)) continue;
        if (!area->area || !other->body) continue;
        Area* self = area->area;
        const AreaShapeContact contact{other->body, bodyShape, areaShape};
        events.emplace_back([self, contact] {
            if (self->shapeExit_) self->shapeExit_(contact);
        });
    }
    for (const Pair& key : lastOverlaps_) {
        if (overlaps.contains(key)) continue;
        const Entry* area = entryOf(key.first);
        const Entry* other = entryOf(key.second);
        if (!area || !other || !area->area) continue;
        Area* self = area->area;
        if (other->body) {
            PhysicsBody* body = other->body;
            events.emplace_back([self, body] {
                if (self->bodyExit_) self->bodyExit_(*body);
            });
        } else if (other->area) {
            Area* left = other->area;
            events.emplace_back([self, left] {
                if (self->areaExit_) self->areaExit_(*left);
            });
        }
    }
    lastOverlaps_ = std::move(overlaps);
    lastShapeOverlaps_ = std::move(shapeOverlaps);

    for (const Event& event : events)
        event();
}

void PhysicsWorld::Draw(PrimitiveRenderer& renderer) const {
    std::vector<ConvexPart> parts;
    for (const Entry& entry : Entries()) {
        Color color = GRAY;
        if (entry.area)
            color = ORANGE;
        else if (dynamic_cast<const CharacterBody*>(entry.object))
            color = LIME;
        else if (entry.body)
            color = SKYBLUE;
        for (const CollisionShape* shape : entry.shapes) {
            parts.clear();
            shape->WorldParts(parts);
            renderer.Reset().TargetScreen().Paint(shape->IsDisabled() ? Fade(GRAY, 0.5f) : color);
            for (const ConvexPart& part : parts) {
                const std::vector<Point2D> outline = part.Outline();
                if (outline.size() == 1)
                    renderer.Circle(outline[0], 3.0f);
                else if (outline.size() == 2)
                    renderer.Line(outline[0], outline[1]);
                else if (!outline.empty())
                    renderer.Polyline(outline, true);
            }
        }
    }
}
