/**
 * @file PhysicsWorld.cpp
 * @brief Świat fizyki na Box2D: odwzorowanie obiektów sceny, MoveAndSlide() na moverze Box2D,
 * zapytania, zdarzenia i podgląd colliderów.
 */
#include <raycade/physics/PhysicsWorld.hpp>

#include <box2d/box2d.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <unordered_map>

namespace {

/** @brief Ile pikseli to jeden „metr” Box2D - jego tolerancje są strojone pod metry. */
constexpr float kPixelsPerMeter = 50.0f;
/** @brief Podkroki solvera Box2D na jeden krok stały. */
constexpr int kSubSteps = 4;

// Bity filtrów Box2D. Warstwy gry zajmują bity 0-31 (kategoria i maska). Bity zapytań są tylko
// w maskach kształtów: zapytanie (promień, mover, overlap) ma kategorię z tymi bitami, więc widzi
// kształt tylko wtedy, gdy jego maska je zawiera. Czujniki ich nie mają - zapytania ich nie widzą.
constexpr std::uint64_t kQueryBit = 1ull << 40; /**< Zwykły kształt widoczny dla zapytań. */
constexpr std::uint64_t kOneWayBit = 1ull
                                     << 41; /**< Platforma jednostronna widoczna dla zapytań. */

/** @brief Najmniejszy cosinus kąta między normalną a „górą” platformy jednostronnej. */
constexpr float kOneWayCos = 0.7f;
/** @brief Zapas wnikania w platformę jednostronną, przy którym nadal zatrzymuje. */
constexpr float kOneWayMargin = 1.0f;
/** @brief Iteracje pętli movera (zderzenie, płaszczyzny, rzut). */
constexpr int kMoverIterations = 5;
/** @brief Najwięcej płaszczyzn styku movera w jednej iteracji. */
constexpr int kMaxPlanes = 16;

b2Vec2 ToB2(Vector2 v) {
    return {v.x, v.y};
}

Vector2 FromB2(b2Vec2 v) {
    return {v.x, v.y};
}

float Dot(Vector2 a, Vector2 b) {
    return a.x * b.x + a.y * b.y;
}

float Length(Vector2 v) {
    return std::sqrt(Dot(v, v));
}

Vector2 Add(Vector2 a, Vector2 b) {
    return {a.x + b.x, a.y + b.y};
}

Vector2 Scale(Vector2 v, float s) {
    return {v.x * s, v.y * s};
}

bool Same(const AffineMatrix2D& a, const AffineMatrix2D& b) {
    constexpr float kEps = 1e-4f;
    return std::fabs(a.a - b.a) < kEps && std::fabs(a.b - b.b) < kEps &&
           std::fabs(a.c - b.c) < kEps && std::fabs(a.d - b.d) < kEps &&
           std::fabs(a.tx - b.tx) < kEps && std::fabs(a.ty - b.ty) < kEps;
}

/** @brief Ciało Box2D nie ma skali - tylko położenie i obrót z macierzy obiektu. */
b2Transform FrameOf(const AffineMatrix2D& world) {
    return {{world.tx, world.ty}, b2MakeRot(std::atan2(world.b, world.a))};
}

/** @brief Macierz bez skali: obrót i przesunięcie ciała Box2D. */
AffineMatrix2D MatrixOf(b2Transform transform) {
    return {transform.q.c, transform.q.s, -transform.q.s,
            transform.q.c, transform.p.x, transform.p.y};
}

/** @brief Box2D wymaga ustawienia jednostek przed utworzeniem pierwszego świata. */
void EnsureUnits() {
    static const bool done = [] {
        b2SetLengthUnitsPerMeter(kPixelsPerMeter);
        return true;
    }();
    (void)done;
}

/** @brief Rodzaj ciała Box2D, którym jest obiekt sceny. */
enum class Kind { Static, Character, Rigid };

/** @brief Kształty Box2D jednego collidera. */
struct ShapeRecord {
    const CollisionShape* shape = nullptr;
    std::vector<b2ShapeId> ids;
    std::vector<float> signature; /**< Geometria i flagi z chwili utworzenia kształtów. */
    b2Filter filter{};
    float friction = -1;
    float bounce = -1;
};

/** @brief Obiekt sceny odwzorowany w Box2D. */
struct BodyRecord {
    CollisionObject* object = nullptr;
    PhysicsBody* body = nullptr;
    Area* area = nullptr;
    RigidBody* rigid = nullptr;
    CharacterBody* character = nullptr;
    Kind kind = Kind::Static;
    b2BodyId id = b2_nullBodyId;
    bool seen = false;
    bool moved = false; /**< StaticBody przesunięty w kodzie - kinematyczny. */
    std::vector<const CollisionShape*> current; /**< Collidery z bieżącego zebrania. */
    std::vector<ShapeRecord> shapes;
    b2Transform stepStart = b2Transform_identity; /**< Położenie Box2D na początku kroku. */
    AffineMatrix2D written{};                     /**< Macierz bryły po ostatnim kroku. */
    bool hasWritten = false;
    float mass = -1;
    Vector2 velocity{}; /**< Prędkość bryły zapisana do obiektu po ostatnim kroku. */
    float angular = 0;
};

/** @brief Kontekst callbacku płaszczyzn movera. */
struct MoverContext {
    const b2Vec2* up = nullptr;
    Vector2 motion{};
    std::array<b2CollisionPlane, kMaxPlanes> planes{};
    std::array<b2ShapeId, kMaxPlanes> shapes{};
    int count = 0;
    bool insideOneWay =
        false; /**< Postać nachodzi na platformę jednostronną, która jej nie trzyma. */
};

/** @brief Czy platforma jednostronna zatrzymuje ruch: normalna w jej „górę” i ruch nie w górę. */
bool OneWayBlocks(const CollisionShape& platform, Vector2 normal, Vector2 motion, float depth) {
    const Vector2 up = platform.OneWayDirection();
    const float down = -Dot(motion, up);
    return Dot(normal, up) >= kOneWayCos && Dot(motion, up) <= 0.0f &&
           depth <= std::max(down, 0.0f) + kOneWayMargin;
}

bool CollectPlane(b2ShapeId shapeId, const b2PlaneResult* result, void* context) {
    auto& mover = *static_cast<MoverContext*>(context);
    if (!result->hit || mover.count >= kMaxPlanes) return true;
    const auto* shape = static_cast<const CollisionShape*>(b2Shape_GetUserData(shapeId));
    if (shape && shape->IsOneWay() &&
        !OneWayBlocks(*shape, FromB2(result->plane.normal), mover.motion, result->plane.offset)) {
        mover.insideOneWay = true;
        return true;
    }
    mover.planes[static_cast<std::size_t>(mover.count)] = {result->plane, FLT_MAX, 0.0f, true};
    mover.shapes[static_cast<std::size_t>(mover.count)] = shapeId;
    ++mover.count;
    return true;
}

/** @brief Platforma jednostronna dla brył: wyłącza styk, który nie jest opadaniem od góry. */
bool PreSolve(b2ShapeId shapeA, b2ShapeId shapeB, b2Manifold* manifold, void*) {
    const auto* a = static_cast<const CollisionShape*>(b2Shape_GetUserData(shapeA));
    const auto* b = static_cast<const CollisionShape*>(b2Shape_GetUserData(shapeB));
    const CollisionShape* platform = a && a->IsOneWay() ? a : (b && b->IsOneWay() ? b : nullptr);
    if (!platform) return true;
    // normalna Box2D wskazuje od A do B - odwracamy, żeby wskazywała od platformy do drugiego
    Vector2 normal = FromB2(manifold->normal);
    if (platform == b) normal = {-normal.x, -normal.y};
    if (Dot(normal, platform->OneWayDirection()) < kOneWayCos) return false;
    for (int i = 0; i < manifold->pointCount; ++i)
        if (manifold->points[i].separation < -kOneWayMargin) return false; // przechodzi przez nią
    return true;
}

} // namespace

struct PhysicsWorld::Impl {
    b2WorldId world = b2_nullWorldId;
    std::vector<BodyRecord> records;
    std::unordered_map<const CollisionObject*, std::size_t> index;

    BodyRecord* Find(const CollisionObject* object) {
        const auto it = index.find(object);
        return it == index.end() ? nullptr : &records[it->second];
    }

    /** @brief Obiekt sceny, do którego należy kształt Box2D. */
    static CollisionObject* ObjectOf(b2ShapeId shape) {
        return static_cast<CollisionObject*>(b2Body_GetUserData(b2Shape_GetBody(shape)));
    }
};

PhysicsWorld::PhysicsWorld(const std::vector<std::unique_ptr<Entity>>& roots)
    : roots_(roots), impl_(std::make_unique<Impl>()) {
    EnsureUnits();
    b2WorldDef def = b2DefaultWorldDef();
    def.gravity = ToB2(gravity_);
    impl_->world = b2CreateWorld(&def);
    b2World_SetPreSolveCallback(impl_->world, PreSolve, nullptr);
}

PhysicsWorld::~PhysicsWorld() {
    if (b2World_IsValid(impl_->world)) b2DestroyWorld(impl_->world);
}

void PhysicsWorld::Gravity(Vector2 gravity) {
    gravity_ = gravity;
    b2World_SetGravity(impl_->world, ToB2(gravity));
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

// ---------------------------------------------------------------------------------------------
// Uzgadnianie świata Box2D z drzewem sceny
// ---------------------------------------------------------------------------------------------

void PhysicsWorld::Collect(const Entity& entity, int owner) const {
    Impl& impl = *impl_;
    // obiekt oznaczony do usunięcia jeszcze istnieje (do końca klatki) - zostaje na liście, ale
    // jego ciało jest wyłączone; dzięki temu obszary i ciała dostają zdarzenie wyjścia
    auto* object = dynamic_cast<CollisionObject*>(const_cast<Entity*>(&entity));
    if (object) {
        BodyRecord* record = impl.Find(object);
        const Kind kind = dynamic_cast<RigidBody*>(object)       ? Kind::Rigid
                          : dynamic_cast<CharacterBody*>(object) ? Kind::Character
                                                                 : Kind::Static;
        if (record && record->kind != kind) { // ten sam adres, inny obiekt
            b2DestroyBody(record->id);
            record->id = b2_nullBodyId;
            record->shapes.clear();
        }
        if (!record) {
            impl.index[object] = impl.records.size();
            impl.records.emplace_back();
            record = &impl.records.back();
        }
        record->object = object;
        record->body = dynamic_cast<PhysicsBody*>(object);
        record->area = dynamic_cast<Area*>(object);
        record->rigid = dynamic_cast<RigidBody*>(object);
        record->character = dynamic_cast<CharacterBody*>(object);
        record->kind = kind;
        record->seen = true;
        record->current.clear();
        owner = static_cast<int>(impl.index[object]);
    } else if (const auto* shape = dynamic_cast<const CollisionShape*>(&entity);
               shape && owner >= 0) {
        if (!entity.destroyed_)
            impl.records[static_cast<std::size_t>(owner)].current.push_back(shape);
    }
    for (const auto& child : entity.children_)
        Collect(*child, owner);
}

void PhysicsWorld::SyncStructure() const {
    if (!dirty_) return;
    Impl& impl = *impl_;
    for (BodyRecord& record : impl.records)
        record.seen = false;
    for (const auto& root : roots_)
        if (!root->screenSpace_) Collect(*root, -1);

    // obiekty usunięte ze sceny - ich ciała znikają (wskaźników już nie wolno dotykać)
    std::vector<BodyRecord> kept;
    kept.reserve(impl.records.size());
    for (BodyRecord& record : impl.records) {
        if (record.seen) {
            kept.push_back(std::move(record));
        } else if (b2Body_IsValid(record.id)) {
            b2DestroyBody(record.id);
        }
    }
    impl.records = std::move(kept);
    impl.index.clear();
    for (std::size_t i = 0; i < impl.records.size(); ++i)
        impl.index[impl.records[i].object] = i;

    for (std::size_t i = 0; i < impl.records.size(); ++i) {
        BodyRecord& record = impl.records[i];
        const AffineMatrix2D world = record.object->WorldTransform();
        if (!b2Body_IsValid(record.id)) {
            b2BodyDef def = b2DefaultBodyDef();
            def.type = record.kind == Kind::Rigid       ? b2_dynamicBody
                       : record.kind == Kind::Character ? b2_kinematicBody
                                                        : b2_staticBody;
            const b2Transform frame = FrameOf(world);
            def.position = frame.p;
            def.rotation = frame.q;
            def.userData = record.object;
            if (record.rigid) {
                def.linearVelocity = ToB2(record.rigid->linearVelocity_);
                def.angularVelocity = record.rigid->angularVelocity_;
            }
            record.id = b2CreateBody(impl.world, &def);
            record.stepStart = frame;
            record.moved = false;
            record.mass = -1;
            record.hasWritten = false;
            record.shapes.clear();
        }
        // obiekt oznaczony do usunięcia nie bierze udziału w kolizjach
        const bool enabled = !record.object->IsDestroyed();
        if (b2Body_IsEnabled(record.id) != enabled) {
            if (enabled)
                b2Body_Enable(record.id);
            else
                b2Body_Disable(record.id);
        }
        SyncShapes(i);
        if (record.rigid) ApplyMass(i);
    }
    dirty_ = false;
}

void PhysicsWorld::SyncShapes(std::size_t index) const {
    BodyRecord& record = impl_->records[index];
    const AffineMatrix2D world = record.object->WorldTransform();
    // kształty Box2D są w układzie ciała bez skali - skala jest wliczona w ich geometrię
    const auto toBody = MatrixOf(FrameOf(world)).Inverse().value_or(AffineMatrix2D{});

    // collidery, które zniknęły
    std::erase_if(record.shapes, [&](ShapeRecord& shapeRecord) {
        if (std::find(record.current.begin(), record.current.end(), shapeRecord.shape) !=
            record.current.end())
            return false;
        for (const b2ShapeId id : shapeRecord.ids)
            if (b2Shape_IsValid(id)) b2DestroyShape(id, false);
        return true;
    });

    std::vector<ConvexPart> parts;
    for (const CollisionShape* shape : record.current) {
        parts.clear();
        if (!shape->IsDisabled()) shape->WorldParts(parts);
        for (ConvexPart& part : parts)
            part = part.Transformed(toBody);

        std::vector<float> signature{
            shape->IsDisabled() ? 1.0f : 0.0f, shape->IsOneWay() ? 1.0f : 0.0f
        };
        for (const ConvexPart& part : parts) {
            signature.push_back(static_cast<float>(part.kind));
            for (const Vector2 point : part.points)
                signature.insert(signature.end(), {point.x, point.y});
            signature.insert(
                signature.end(), {part.center.x, part.center.y, part.axisX.x, part.axisX.y,
                                  part.axisY.x, part.axisY.y}
            );
        }

        auto it = std::find_if(record.shapes.begin(), record.shapes.end(), [&](const auto& s) {
            return s.shape == shape;
        });
        if (it == record.shapes.end()) {
            record.shapes.push_back({shape, {}, {}, {}, -1, -1});
            it = std::prev(record.shapes.end());
        }
        ShapeRecord& shapeRecord = *it;

        // filtr: warstwy gry + bity zapytań (czujniki są dla zapytań niewidoczne)
        b2Filter filter = b2DefaultFilter();
        filter.categoryBits = record.object->CollisionLayer();
        filter.maskBits = record.object->CollisionMask();
        if (!record.area) filter.maskBits |= shape->IsOneWay() ? kOneWayBit : kQueryBit;
        const float friction = record.body ? record.body->friction_ : 0.0f;
        const float bounce = record.body ? record.body->bounce_ : 0.0f;

        // porównanie z tolerancją - przeliczenie do układu ciała daje co krok minimalnie inne
        // liczby, a każda przebudowa kasuje styki Box2D (i stos brył zaczyna drgać)
        const bool same = shapeRecord.signature.size() == signature.size() &&
                          std::equal(
                              signature.begin(), signature.end(), shapeRecord.signature.begin(),
                              [](float a, float b) { return std::fabs(a - b) < 1e-3f; }
                          );
        if (!same) {
            for (const b2ShapeId id : shapeRecord.ids)
                if (b2Shape_IsValid(id)) b2DestroyShape(id, false);
            shapeRecord.ids.clear();
            shapeRecord.signature = std::move(signature);

            b2ShapeDef def = b2DefaultShapeDef();
            def.userData = const_cast<CollisionShape*>(shape);
            def.filter = filter;
            def.material.friction = friction;
            def.material.restitution = bounce;
            def.isSensor = record.area != nullptr;
            def.enableSensorEvents = true;
            def.enablePreSolveEvents = record.rigid != nullptr;
            def.updateBodyMass = false;
            def.density = 1.0f;
            for (const ConvexPart& part : parts) {
                if (part.kind == ConvexPart::Kind::Ellipse) {
                    const float rx = Length(part.axisX);
                    const float ry = Length(part.axisY);
                    if (std::fabs(rx - ry) < 1e-3f &&
                        std::fabs(Dot(part.axisX, part.axisY)) < 1e-3f) {
                        const b2Circle circle{ToB2(part.center), rx};
                        shapeRecord.ids.push_back(b2CreateCircleShape(record.id, &def, &circle));
                        continue;
                    }
                    // elipsa: ośmiokąt wpisany (Box2D nie ma elipsy)
                    std::array<b2Vec2, 8> octagon{};
                    for (int k = 0; k < 8; ++k) {
                        const float t = 2.0f * PI * static_cast<float>(k) / 8.0f;
                        octagon[static_cast<std::size_t>(k)] = ToB2(
                            Add(part.center,
                                Add(Scale(part.axisX, std::cos(t)), Scale(part.axisY, std::sin(t))))
                        );
                    }
                    const b2Hull hull = b2ComputeHull(octagon.data(), 8);
                    if (hull.count < 3) continue;
                    const b2Polygon polygon = b2MakePolygon(&hull, 0.0f);
                    shapeRecord.ids.push_back(b2CreatePolygonShape(record.id, &def, &polygon));
                    continue;
                }
                const std::vector<Vector2>& points = part.points;
                if (points.size() == 1) {
                    const b2Circle circle{ToB2(points[0]), 0.5f};
                    shapeRecord.ids.push_back(b2CreateCircleShape(record.id, &def, &circle));
                } else if (points.size() == 2) {
                    const b2Segment segment{ToB2(points[0]), ToB2(points[1])};
                    shapeRecord.ids.push_back(b2CreateSegmentShape(record.id, &def, &segment));
                } else {
                    // wielokąt wypukły: po B2_MAX_POLYGON_VERTICES wierzchołków (wachlarz z p0)
                    for (std::size_t first = 1; first + 1 < points.size();
                         first += B2_MAX_POLYGON_VERTICES - 2) {
                        std::array<b2Vec2, B2_MAX_POLYGON_VERTICES> chunk{};
                        int count = 0;
                        chunk[static_cast<std::size_t>(count++)] = ToB2(points[0]);
                        for (std::size_t k = first;
                             k < points.size() && count < B2_MAX_POLYGON_VERTICES; ++k)
                            chunk[static_cast<std::size_t>(count++)] = ToB2(points[k]);
                        const b2Hull hull = b2ComputeHull(chunk.data(), count);
                        if (hull.count < 3) continue;
                        const b2Polygon polygon = b2MakePolygon(&hull, 0.0f);
                        shapeRecord.ids.push_back(b2CreatePolygonShape(record.id, &def, &polygon));
                    }
                }
            }
            shapeRecord.filter = filter;
            shapeRecord.friction = friction;
            shapeRecord.bounce = bounce;
            record.mass = -1; // masa do przeliczenia
            continue;
        }
        // ta sama geometria - tylko filtr i materiał, jeśli się zmieniły
        if (filter.categoryBits != shapeRecord.filter.categoryBits ||
            filter.maskBits != shapeRecord.filter.maskBits) {
            for (const b2ShapeId id : shapeRecord.ids)
                b2Shape_SetFilter(id, filter);
            shapeRecord.filter = filter;
        }
        if (friction != shapeRecord.friction || bounce != shapeRecord.bounce) {
            for (const b2ShapeId id : shapeRecord.ids) {
                b2Shape_SetFriction(id, friction);
                b2Shape_SetRestitution(id, bounce);
            }
            shapeRecord.friction = friction;
            shapeRecord.bounce = bounce;
        }
    }
}

void PhysicsWorld::ApplyMass(std::size_t index) const {
    BodyRecord& record = impl_->records[index];
    const float mass = record.rigid->mass_;
    if (record.mass == mass) return;
    // gęstość 1 -> masa = pole; potem gęstość = Mass() / pole
    for (const ShapeRecord& shape : record.shapes)
        for (const b2ShapeId id : shape.ids)
            b2Shape_SetDensity(id, 1.0f, false);
    b2Body_ApplyMassFromShapes(record.id);
    const float area = b2Body_GetMassData(record.id).mass;
    if (area > 1e-3f) {
        for (const ShapeRecord& shape : record.shapes)
            for (const b2ShapeId id : shape.ids)
                b2Shape_SetDensity(id, mass / area, false);
        b2Body_ApplyMassFromShapes(record.id);
    } else {
        // bez pola (punkty, odcinki): masa w środku, moment jak kulki o promieniu ~14 px
        b2Body_SetMassData(record.id, {mass, {0.0f, 0.0f}, mass * 100.0f});
    }
    record.mass = mass;
}

void PhysicsWorld::SyncTransforms() const {
    SyncStructure();
    for (BodyRecord& record : impl_->records) {
        if (!b2Body_IsValid(record.id)) continue;
        const AffineMatrix2D world = record.object->WorldTransform();
        const b2Transform frame = FrameOf(world);
        const b2Transform now = b2Body_GetTransform(record.id);
        const bool differs =
            std::fabs(frame.p.x - now.p.x) > 1e-4f || std::fabs(frame.p.y - now.p.y) > 1e-4f ||
            std::fabs(frame.q.c - now.q.c) > 1e-5f || std::fabs(frame.q.s - now.q.s) > 1e-5f;
        if (!differs) continue;
        if (record.kind == Kind::Rigid) {
            // bryła przesunięta w kodzie (Position(), Rotate()) - teleport
            if (record.hasWritten && Same(world, record.written)) continue;
            b2Body_SetTransform(record.id, frame.p, frame.q);
            b2Body_SetAwake(record.id, true);
            record.written = world;
            record.hasWritten = true;
            continue;
        }
        if (record.kind == Kind::Static && !record.moved) {
            // StaticBody przesuwany w kodzie staje się kinematyczny - ma prędkość i wiezie bryły
            b2Body_SetType(record.id, b2_kinematicBody);
            record.moved = true;
        }
        // obiekty kinematyczne: zapytania widzą bieżące położenie; prędkość dostaną w kroku
        b2Body_SetTransform(record.id, frame.p, frame.q);
    }
}

void PhysicsWorld::PrepareStep() {
    SyncTransforms();
    for (BodyRecord& record : impl_->records) {
        if (!b2Body_IsValid(record.id)) continue;
        if (record.kind != Kind::Rigid) {
            if (record.kind == Kind::Static && !record.moved) continue;
            // od położenia z początku kroku do bieżącego - Box2D liczy z tego prędkość, więc
            // jadąca platforma wiezie bryły, a postać je popycha
            const b2Transform target = FrameOf(record.object->WorldTransform());
            b2Body_SetTransform(record.id, record.stepStart.p, record.stepStart.q);
            b2Body_SetTargetTransform(record.id, target, delta_);
            continue;
        }
        RigidBody& rigid = *record.rigid;
        b2Body_SetGravityScale(record.id, rigid.gravityScale_);
        b2Body_SetLinearDamping(record.id, rigid.linearDamp_);
        b2Body_SetAngularDamping(record.id, rigid.angularDamp_);
        if (b2Body_IsFixedRotation(record.id) != rigid.lockRotation_)
            b2Body_SetFixedRotation(record.id, rigid.lockRotation_);
        // prędkość zmieniona w kodzie od ostatniego kroku
        if (rigid.linearVelocity_.x != record.velocity.x ||
            rigid.linearVelocity_.y != record.velocity.y)
            b2Body_SetLinearVelocity(record.id, ToB2(rigid.linearVelocity_));
        if (rigid.angularVelocity_ != record.angular)
            b2Body_SetAngularVelocity(record.id, rigid.angularVelocity_);
        for (const RigidBody::Pending& pending : rigid.pending_) {
            const b2Vec2 vector = ToB2(pending.vector);
            if (pending.impulse) {
                if (pending.central)
                    b2Body_ApplyLinearImpulseToCenter(record.id, vector, true);
                else
                    b2Body_ApplyLinearImpulse(record.id, vector, ToB2(pending.point), true);
            } else if (pending.central) {
                b2Body_ApplyForceToCenter(record.id, vector, true);
            } else {
                b2Body_ApplyForce(record.id, vector, ToB2(pending.point), true);
            }
        }
        if (rigid.torque_ != 0.0f) b2Body_ApplyTorque(record.id, rigid.torque_, true);
        if (rigid.torqueImpulse_ != 0.0f)
            b2Body_ApplyAngularImpulse(record.id, rigid.torqueImpulse_, true);
        rigid.pending_.clear();
        rigid.torque_ = 0.0f;
        rigid.torqueImpulse_ = 0.0f;
    }
}

void PhysicsWorld::ReadBack() {
    for (BodyRecord& record : impl_->records) {
        if (!b2Body_IsValid(record.id)) continue;
        record.stepStart = b2Body_GetTransform(record.id);
        if (record.kind != Kind::Rigid) continue;
        RigidBody& rigid = *record.rigid;
        // nowa macierz: położenie i obrót z Box2D, skala obiektu bez zmian
        const AffineMatrix2D world = rigid.WorldTransform();
        const b2Transform before = FrameOf(world);
        const b2Transform after = record.stepStart;
        const b2Rot turn = b2InvMulRot(before.q, after.q);
        AffineMatrix2D next = AffineMatrix2D::Rotation(b2Rot_GetAngle(turn)) * world;
        next.tx = after.p.x;
        next.ty = after.p.y;
        const Entity* parent = rigid.Parent();
        const AffineMatrix2D local =
            parent ? parent->WorldTransform().Inverse().value_or(AffineMatrix2D{}) * next : next;
        rigid.transform_ = local;
        record.written = rigid.WorldTransform();
        record.hasWritten = true;

        rigid.linearVelocity_ = FromB2(b2Body_GetLinearVelocity(record.id));
        rigid.angularVelocity_ = b2Body_GetAngularVelocity(record.id);
        rigid.centerOfMass_ = FromB2(b2Body_GetWorldCenterOfMass(record.id));
        record.velocity = rigid.linearVelocity_;
        record.angular = rigid.angularVelocity_;
    }
}

// ---------------------------------------------------------------------------------------------
// Krok
// ---------------------------------------------------------------------------------------------

void PhysicsWorld::BeginFrame(float dt) {
    delta_ = dt;
}

void PhysicsWorld::BeginStep(float step) {
    delta_ = step;
    dirty_ = true;
    SyncStructure();
    // stan przed krokiem - początek interpolacji rysowania
    for (BodyRecord& record : impl_->records) {
        record.object->previous_ = record.object->transform_;
        record.object->inStep_ = true;
    }
}

void PhysicsWorld::Record(PhysicsBody& body, const Collision& hit) {
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

void PhysicsWorld::EndStep() {
    dirty_ = true; // obiekty mogły zostać dodane albo usunięte w FixedTick()
    PrepareStep();
    b2World_Step(impl_->world, delta_, kSubSteps);
    ReadBack();

    Impl& impl = *impl_;
    // zderzenia brył z Box2D (styki z punktami); postacie dopisują swoje w MoveAndSlide()
    std::vector<b2ContactData> contactData;
    for (BodyRecord& record : impl.records) {
        if (!record.rigid || !b2Body_IsValid(record.id) || !b2Body_IsEnabled(record.id)) continue;
        const int capacity = b2Body_GetContactCapacity(record.id);
        contactData.resize(static_cast<std::size_t>(std::max(capacity, 1)));
        const int count = b2Body_GetContactData(record.id, contactData.data(), capacity);
        for (int k = 0; k < count; ++k) {
            const b2ContactData& data = contactData[static_cast<std::size_t>(k)];
            if (data.manifold.pointCount == 0) continue;
            const bool mineIsA = Impl::ObjectOf(data.shapeIdA) == record.object;
            const b2ShapeId mine = mineIsA ? data.shapeIdA : data.shapeIdB;
            const b2ShapeId theirs = mineIsA ? data.shapeIdB : data.shapeIdA;
            auto* other = dynamic_cast<PhysicsBody*>(Impl::ObjectOf(theirs));
            if (!other) continue;
            float depth = 0.0f;
            for (int p = 0; p < data.manifold.pointCount; ++p)
                depth = std::max(depth, -data.manifold.points[p].separation);
            // normalna Box2D: od A do B; w Collision - od drugiego ciała do mnie
            Vector2 normal = FromB2(data.manifold.normal);
            if (mineIsA) normal = {-normal.x, -normal.y};
            Record(
                *record.rigid,
                Collision{
                    other, static_cast<const CollisionShape*>(b2Shape_GetUserData(mine)),
                    static_cast<const CollisionShape*>(b2Shape_GetUserData(theirs)), normal, depth
                }
            );
        }
    }

    // obiekty usunięte ze sceny przed tym krokiem nie istnieją - nie wolno ich dotykać
    const auto present = [&](const CollisionObject* object) {
        return impl.Find(object) != nullptr;
    };

    // najpierw zebranie zdarzeń, potem wywołanie: callback może usuwać i dodawać obiekty
    std::vector<std::function<void()>> events;

    for (const auto& [key, contact] : contacts_) {
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

    // obszary: stan nachodzenia czujników po kroku
    std::set<Pair> overlaps;
    std::set<ShapePair> shapeOverlaps;
    std::vector<b2ShapeId> visitors;
    for (BodyRecord& record : impl.records) {
        if (!record.area || record.object->IsDestroyed()) continue;
        Area* self = record.area;
        for (const ShapeRecord& areaShape : record.shapes)
            for (const b2ShapeId sensor : areaShape.ids) {
                const int capacity = b2Shape_GetSensorCapacity(sensor);
                if (capacity <= 0) continue;
                visitors.resize(static_cast<std::size_t>(capacity));
                const int count = b2Shape_GetSensorOverlaps(sensor, visitors.data(), capacity);
                for (int k = 0; k < count; ++k) {
                    const b2ShapeId visitor = visitors[static_cast<std::size_t>(k)];
                    if (!b2Shape_IsValid(visitor)) continue;
                    CollisionObject* other = Impl::ObjectOf(visitor);
                    if (!other || other == record.object || other->IsDestroyed()) continue;
                    if (!record.object->CollidesWith(*other)) continue;
                    const auto* otherShape =
                        static_cast<const CollisionShape*>(b2Shape_GetUserData(visitor));
                    const Pair key{record.object, other};
                    if (overlaps.insert(key).second && !lastOverlaps_.contains(key)) {
                        if (auto* body = dynamic_cast<PhysicsBody*>(other)) {
                            events.emplace_back([self, body] {
                                if (self->bodyEnter_) self->bodyEnter_(*body);
                            });
                        } else if (auto* entered = dynamic_cast<Area*>(other)) {
                            events.emplace_back([self, entered] {
                                if (self->areaEnter_) self->areaEnter_(*entered);
                            });
                        }
                    }
                    auto* body = dynamic_cast<PhysicsBody*>(other);
                    if (!body) continue;
                    const ShapePair shapeKey{record.object, areaShape.shape, other, otherShape};
                    if (shapeOverlaps.insert(shapeKey).second &&
                        !lastShapeOverlaps_.contains(shapeKey)) {
                        const AreaShapeContact contact{body, otherShape, areaShape.shape};
                        events.emplace_back([self, contact] {
                            if (self->shapeEnter_) self->shapeEnter_(contact);
                        });
                    }
                }
            }
    }
    const auto ownsShape = [&](const CollisionObject* object, const CollisionShape* shape) {
        const BodyRecord* record = impl.Find(object);
        return record && std::find(record->current.begin(), record->current.end(), shape) !=
                             record->current.end();
    };
    for (const ShapePair& key : lastShapeOverlaps_) {
        if (shapeOverlaps.contains(key)) continue;
        const auto& [areaObject, areaShape, bodyObject, bodyShape] = key;
        if (!ownsShape(areaObject, areaShape) || !ownsShape(bodyObject, bodyShape)) continue;
        const BodyRecord* area = impl.Find(areaObject);
        const BodyRecord* other = impl.Find(bodyObject);
        if (!area->area || !other->body) continue;
        Area* self = area->area;
        const AreaShapeContact contact{other->body, bodyShape, areaShape};
        events.emplace_back([self, contact] {
            if (self->shapeExit_) self->shapeExit_(contact);
        });
    }
    for (const Pair& key : lastOverlaps_) {
        if (overlaps.contains(key)) continue;
        const BodyRecord* area = impl.Find(key.first);
        const BodyRecord* other = impl.Find(key.second);
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

    // stan po kroku - koniec interpolacji (obiekt dodany w trakcie kroku jej nie ma)
    for (BodyRecord& record : impl.records) {
        record.object->stepped_ = record.object->transform_;
        record.object->interpolated_ = record.object->inStep_;
        record.object->inStep_ = false;
    }

    for (const auto& event : events)
        event();
}

// ---------------------------------------------------------------------------------------------
// Postać: mover Box2D
// ---------------------------------------------------------------------------------------------

bool PhysicsWorld::MoveAndSlide(CharacterBody& body) {
    const bool wasOnFloor = body.onFloor_;
    body.collisions_.clear();
    body.onFloor_ = body.onWall_ = body.onCeiling_ = false;

    SyncTransforms();
    BodyRecord* record = impl_->Find(&body);
    const Vector2 motion{body.velocity_.x * delta_, body.velocity_.y * delta_};
    if (!record || record->shapes.empty() || body.IsDestroyed()) {
        body.MoveInScene(motion); // bez colliderów - zwykły ruch
        return false;
    }

    // kapsuła movera z obrysu colliderów (Box2D przesuwa postać kapsułą)
    std::vector<ConvexPart> parts;
    for (const CollisionShape* shape : record->current)
        if (!shape->IsDisabled()) shape->WorldParts(parts);
    float left = INFINITY, top = INFINITY, right = -INFINITY, bottom = -INFINITY;
    for (const ConvexPart& part : parts)
        for (const Vector2 direction :
             {Vector2{1, 0}, Vector2{-1, 0}, Vector2{0, 1}, Vector2{0, -1}}) {
            const Vector2 point = part.Support(direction);
            left = std::min(left, point.x);
            right = std::max(right, point.x);
            top = std::min(top, point.y);
            bottom = std::max(bottom, point.y);
        }
    if (parts.empty() || !std::isfinite(left)) {
        body.MoveInScene(motion);
        return false;
    }
    const Vector2 origin = body.WorldPosition();
    const float width = right - left;
    const float height = bottom - top;
    const float radius = std::max(0.5f, std::min(width, height) * 0.5f);
    const Vector2 middle{(left + right) * 0.5f, (top + bottom) * 0.5f};
    const float half = std::max(0.01f, std::max(width, height) * 0.5f - radius);
    const Vector2 axis = height >= width ? Vector2{0, half} : Vector2{half, 0};
    const Vector2 offset1{middle.x - axis.x - origin.x, middle.y - axis.y - origin.y};
    const Vector2 offset2{middle.x + axis.x - origin.x, middle.y + axis.y - origin.y};
    const auto capsule = [&](Vector2 at) {
        return b2Capsule{ToB2(Add(at, offset1)), ToB2(Add(at, offset2)), radius};
    };

    const Vector2 up = body.up_;
    const b2Vec2 upB2 = ToB2(up);
    const float floorCos = std::cos(body.floorMaxAngle_);
    const b2QueryFilter collideFilter{kQueryBit | kOneWayBit, body.mask_};

    // postać nie może zderzać się sama ze sobą - jej ciało jest wyłączone na czas ruchu
    b2Body_Disable(record->id);

    // wożenie przez platformę: przesunięcie punktu pod postacią od jej ostatniego ruchu
    body.platformVelocity_ = {};
    const PhysicsBody* floorBody = wasOnFloor ? body.floorBody_ : nullptr;
    const bool teleported = std::fabs(origin.x - body.floorWorld_.x) > 0.01f ||
                            std::fabs(origin.y - body.floorWorld_.y) > 0.01f;
    Vector2 position = origin;
    if (floorBody && !teleported && impl_->Find(floorBody)) {
        const Vector2 now = floorBody->World(body.floorLocal_);
        const Vector2 carry{now.x - body.floorWorld_.x, now.y - body.floorWorld_.y};
        position = Add(position, carry);
        if (delta_ > 0.0f) body.platformVelocity_ = Scale(carry, 1.0f / delta_);
    }

    MoverContext context;
    context.up = &upB2;
    std::vector<std::pair<b2ShapeId, b2Plane>> touched; // do zderzeń i zdarzeń
    // jedna pętla movera: zderzenie -> płaszczyzny -> rzut kapsuły
    const auto slide = [&](Vector2 translation) {
        const Vector2 target = Add(position, translation);
        for (int iteration = 0; iteration < kMoverIterations; ++iteration) {
            context.count = 0;
            context.insideOneWay = false;
            context.motion = translation;
            const b2Capsule mover = capsule(position);
            b2World_CollideMover(impl_->world, &mover, collideFilter, CollectPlane, &context);
            for (int k = 0; k < context.count; ++k)
                touched.emplace_back(
                    context.shapes[static_cast<std::size_t>(k)],
                    context.planes[static_cast<std::size_t>(k)].plane
                );
            const Vector2 remaining{target.x - position.x, target.y - position.y};
            const b2PlaneSolverResult solved =
                b2SolvePlanes(ToB2(remaining), context.planes.data(), context.count);
            // platformy jednostronne blokują rzut tylko przy ruchu w dół i gdy postać na nie nie
            // nachodzi (inaczej zatrzymałaby się w środku platformy, przez którą przeskakuje)
            const bool oneWay = Dot(remaining, up) <= 0.0f && !context.insideOneWay;
            const b2QueryFilter castFilter{kQueryBit | (oneWay ? kOneWayBit : 0), body.mask_};
            const float fraction =
                b2World_CastMover(impl_->world, &mover, solved.translation, castFilter);
            const Vector2 step = Scale(FromB2(solved.translation), fraction);
            position = Add(position, step);
            if (Dot(step, step) < 1e-4f) break;
        }
    };

    const float vertical = Dot(motion, up);
    if (wasOnFloor && vertical <= 0.0f) {
        // na podłodze: ruch w bok ślizga się po powierzchni (także pod górę pochylni), a spadek
        // w dół jest rzutem - bez zsuwania się z pochylni pod wpływem grawitacji
        // bez ruchu w bok nie ma czego rozwiązywać - wypychanie z zapasu Box2D wzdłuż normalnej
        // pochylni przesuwałoby stojącą postać w dół zbocza
        const Vector2 lateral{motion.x - up.x * vertical, motion.y - up.y * vertical};
        if (Dot(lateral, lateral) > 1e-8f) {
            slide(lateral);
        } else {
            // stoi: wypchnięcie z zapasu Box2D pionowo w górę (z podłogi) i wzdłuż normalnej
            // (ze ścian) - bez tego rzut w dół startowałby z wnikania i przelatywał przez podłogę
            context.count = 0;
            context.motion = Scale(up, -1.0f);
            const b2Capsule mover = capsule(position);
            b2World_CollideMover(impl_->world, &mover, collideFilter, CollectPlane, &context);
            float lift = 0.0f;
            for (int k = 0; k < context.count; ++k) {
                const b2Plane plane = context.planes[static_cast<std::size_t>(k)].plane;
                const Vector2 normal = FromB2(plane.normal);
                const float upness = Dot(normal, up);
                if (upness >= floorCos)
                    lift = std::max(lift, plane.offset / upness);
                else
                    position = Add(position, Scale(normal, plane.offset));
            }
            position = Add(position, Scale(up, lift));
        }
        const float drop = -vertical + body.floorSnapLength_;
        const b2Capsule mover = capsule(position);
        const b2QueryFilter castFilter{kQueryBit | kOneWayBit, body.mask_};
        const Vector2 down = Scale(up, -drop);
        const float fraction = b2World_CastMover(impl_->world, &mover, ToB2(down), castFilter);
        position = fraction < 1.0f ? Add(position, Scale(down, fraction))
                                   : Add(position, Scale(up, vertical));
    } else {
        slide(motion);
    }

    // styk po ruchu: podłoga, ściany, sufit
    context.count = 0;
    context.motion = Dot(motion, up) <= 0.0f ? Scale(up, -1.0f) : motion;
    const b2Capsule final = capsule(position);
    b2World_CollideMover(impl_->world, &final, collideFilter, CollectPlane, &context);
    std::array<b2CollisionPlane, kMaxPlanes> clipPlanes{};
    int clipCount = 0;
    const PhysicsBody* standingOn = nullptr;
    for (int k = 0; k < context.count; ++k) {
        const b2Plane plane = context.planes[static_cast<std::size_t>(k)].plane;
        touched.emplace_back(context.shapes[static_cast<std::size_t>(k)], plane);
        const float upness = Dot(FromB2(plane.normal), up);
        if (upness >= floorCos) {
            body.onFloor_ = true;
            body.floorNormal_ = FromB2(plane.normal);
            standingOn = dynamic_cast<const PhysicsBody*>(
                Impl::ObjectOf(context.shapes[static_cast<std::size_t>(k)])
            );
        } else {
            if (upness <= -floorCos)
                body.onCeiling_ = true;
            else
                body.onWall_ = true;
            clipPlanes[static_cast<std::size_t>(clipCount++)] =
                context.planes[static_cast<std::size_t>(k)];
        }
    }

    b2Body_Enable(record->id);
    body.MoveInScene({position.x - origin.x, position.y - origin.y});

    // prędkość: na podłodze bez składowej w dół, na ścianach i suficie - wzdłuż powierzchni
    if (body.onFloor_) {
        const float into = Dot(body.velocity_, up);
        if (into < 0.0f)
            body.velocity_ = {body.velocity_.x - up.x * into, body.velocity_.y - up.y * into};
    }
    if (clipCount > 0)
        body.velocity_ = FromB2(b2ClipVector(ToB2(body.velocity_), clipPlanes.data(), clipCount));

    // zderzenia (po jednym na collider innego ciała) i zdarzenia
    const CollisionShape* myShape = record->current.empty() ? nullptr : record->current.front();
    for (const auto& [shapeId, plane] : touched) {
        if (!b2Shape_IsValid(shapeId)) continue;
        auto* other = dynamic_cast<PhysicsBody*>(Impl::ObjectOf(shapeId));
        if (!other || other == &body) continue;
        const auto* otherShape = static_cast<const CollisionShape*>(b2Shape_GetUserData(shapeId));
        const bool known =
            std::any_of(body.collisions_.begin(), body.collisions_.end(), [&](const Collision& c) {
                return c.otherShape == otherShape;
            });
        if (known) continue;
        const Collision hit{
            other, myShape, otherShape, FromB2(plane.normal), std::max(0.0f, plane.offset)
        };
        body.collisions_.push_back(hit);
        Record(body, hit);
    }

    // zapamiętanie podłogi do wożenia w następnym ruchu
    body.floorBody_ = body.onFloor_ ? standingOn : nullptr;
    if (body.floorBody_) {
        body.floorWorld_ = body.WorldPosition();
        body.floorLocal_ = body.floorBody_->ToLocal(body.floorWorld_);
    }
    // zejście z jadącej platformy (skok, krawędź) - postać zachowuje jej prędkość
    if (wasOnFloor && !body.onFloor_) body.velocity_ = Add(body.velocity_, body.platformVelocity_);
    return !body.collisions_.empty();
}

// ---------------------------------------------------------------------------------------------
// Zapytania
// ---------------------------------------------------------------------------------------------

std::vector<PhysicsBody*> PhysicsWorld::OverlappingBodies(const Area& area) {
    SyncStructure();
    std::vector<PhysicsBody*> bodies;
    const BodyRecord* record = impl_->Find(&area);
    if (!record) return bodies;
    std::vector<b2ShapeId> visitors;
    for (const ShapeRecord& shape : record->shapes)
        for (const b2ShapeId sensor : shape.ids) {
            const int capacity = b2Shape_GetSensorCapacity(sensor);
            if (capacity <= 0) continue;
            visitors.resize(static_cast<std::size_t>(capacity));
            const int count = b2Shape_GetSensorOverlaps(sensor, visitors.data(), capacity);
            for (int k = 0; k < count; ++k) {
                if (!b2Shape_IsValid(visitors[static_cast<std::size_t>(k)])) continue;
                auto* body = dynamic_cast<PhysicsBody*>(
                    Impl::ObjectOf(visitors[static_cast<std::size_t>(k)])
                );
                if (!body || body->IsDestroyed() || !area.CollidesWith(*body)) continue;
                if (std::find(bodies.begin(), bodies.end(), body) == bodies.end())
                    bodies.push_back(body);
            }
        }
    return bodies;
}

std::vector<Area*> PhysicsWorld::OverlappingAreas(const Area& area) {
    SyncStructure();
    std::vector<Area*> areas;
    const BodyRecord* record = impl_->Find(&area);
    if (!record) return areas;
    std::vector<b2ShapeId> visitors;
    for (const ShapeRecord& shape : record->shapes)
        for (const b2ShapeId sensor : shape.ids) {
            const int capacity = b2Shape_GetSensorCapacity(sensor);
            if (capacity <= 0) continue;
            visitors.resize(static_cast<std::size_t>(capacity));
            const int count = b2Shape_GetSensorOverlaps(sensor, visitors.data(), capacity);
            for (int k = 0; k < count; ++k) {
                if (!b2Shape_IsValid(visitors[static_cast<std::size_t>(k)])) continue;
                auto* other =
                    dynamic_cast<Area*>(Impl::ObjectOf(visitors[static_cast<std::size_t>(k)]));
                if (!other || other == &area || other->IsDestroyed() || !area.CollidesWith(*other))
                    continue;
                if (std::find(areas.begin(), areas.end(), other) == areas.end())
                    areas.push_back(other);
            }
        }
    return areas;
}

namespace {

/** @brief Kontekst callbacku promienia. */
struct RayContext {
    const CollisionObject* exclude = nullptr;
    Vector2 direction{};
    std::optional<RayHit> hit;
};

float RayCallback(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context) {
    auto& ray = *static_cast<RayContext*>(context);
    auto* body = dynamic_cast<PhysicsBody*>(
        static_cast<CollisionObject*>(b2Body_GetUserData(b2Shape_GetBody(shapeId)))
    );
    const auto* shape = static_cast<const CollisionShape*>(b2Shape_GetUserData(shapeId));
    if (!body || body == ray.exclude || body->IsDestroyed()) return -1.0f; // pomiń
    // platforma jednostronna zatrzymuje tylko promień lecący z jej „góry”
    if (shape && shape->IsOneWay() && Dot(ray.direction, shape->OneWayDirection()) >= 0.0f)
        return -1.0f;
    ray.hit = RayHit{body, shape, FromB2(point), FromB2(normal), fraction * Length(ray.direction)};
    return fraction; // dalej szukamy tylko bliżej
}

/** @brief Kontekst callbacku nachodzenia. */
struct OverlapContext {
    const CollisionObject* exclude = nullptr;
    std::vector<PhysicsBody*> bodies;
};

bool OverlapCallback(b2ShapeId shapeId, void* context) {
    auto& overlap = *static_cast<OverlapContext*>(context);
    auto* body = dynamic_cast<PhysicsBody*>(
        static_cast<CollisionObject*>(b2Body_GetUserData(b2Shape_GetBody(shapeId)))
    );
    if (!body || body == overlap.exclude || body->IsDestroyed()) return true;
    if (std::find(overlap.bodies.begin(), overlap.bodies.end(), body) == overlap.bodies.end())
        overlap.bodies.push_back(body);
    return true;
}

} // namespace

std::optional<RayHit> PhysicsWorld::Raycast(
    Vector2 from,
    Vector2 to,
    std::uint32_t mask,
    const CollisionObject* exclude
) const {
    SyncTransforms();
    RayContext context;
    context.exclude = exclude;
    context.direction = {to.x - from.x, to.y - from.y};
    const b2QueryFilter filter{kQueryBit | kOneWayBit, mask};
    b2World_CastRay(
        impl_->world, ToB2(from), ToB2(context.direction), filter, RayCallback, &context
    );
    return context.hit;
}

std::vector<PhysicsBody*> PhysicsWorld::Overlap(
    const ConvexPart& part,
    std::uint32_t mask,
    const CollisionObject* exclude
) const {
    SyncTransforms();
    b2ShapeProxy proxy{};
    if (part.kind == ConvexPart::Kind::Ellipse) {
        const b2Vec2 center = ToB2(part.center);
        proxy = b2MakeProxy(&center, 1, std::max(Length(part.axisX), Length(part.axisY)));
    } else {
        std::array<b2Vec2, B2_MAX_POLYGON_VERTICES> points{};
        const int count =
            std::min<int>(static_cast<int>(part.points.size()), B2_MAX_POLYGON_VERTICES);
        for (int k = 0; k < count; ++k)
            points[static_cast<std::size_t>(k)] = ToB2(part.points[static_cast<std::size_t>(k)]);
        proxy = b2MakeProxy(points.data(), count, 0.0f);
    }
    OverlapContext context;
    context.exclude = exclude;
    const b2QueryFilter filter{kQueryBit | kOneWayBit, mask};
    b2World_OverlapShape(impl_->world, &proxy, filter, OverlapCallback, &context);
    return context.bodies;
}

// ---------------------------------------------------------------------------------------------
// Podgląd
// ---------------------------------------------------------------------------------------------

void PhysicsWorld::Draw(PrimitiveRenderer& renderer) const {
    // własne przejście po drzewie (bez Box2D) - rysowany jest kształt figury, nie przybliżenie
    std::vector<ConvexPart> parts;
    std::function<void(const Entity&)> visit = [&](const Entity& entity) {
        if (entity.destroyed_) return;
        if (const auto* object = dynamic_cast<const CollisionObject*>(&entity)) {
            Color color = SKYBLUE;
            if (dynamic_cast<const Area*>(object))
                color = ORANGE;
            else if (dynamic_cast<const CharacterBody*>(object))
                color = LIME;
            else if (dynamic_cast<const RigidBody*>(object))
                color = VIOLET;
            std::vector<const CollisionShape*> shapes;
            CollectShapes(*object, shapes);
            for (const CollisionShape* shape : shapes) {
                parts.clear();
                shape->WorldParts(parts);
                renderer.Reset().TargetScreen().Paint(
                    shape->IsDisabled() ? Fade(GRAY, 0.5f) : color
                );
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
            if (const auto* rigid = dynamic_cast<const RigidBody*>(object))
                renderer.Paint(VIOLET).Circle(rigid->CenterOfMass(), 2.0f);
        }
        for (const auto& child : entity.children_)
            visit(*child);
    };
    for (const auto& root : roots_)
        if (!root->screenSpace_) visit(*root);
}
