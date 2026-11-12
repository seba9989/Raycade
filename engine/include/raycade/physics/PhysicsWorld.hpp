/**
 * @file PhysicsWorld.hpp
 * @brief Klasa PhysicsWorld - kolizje jednej sceny: ruch postaci, obszary i zdarzenia.
 */
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include <raycade/entity/Entity.hpp>
#include <raycade/physics/Area.hpp>
#include <raycade/physics/CharacterBody.hpp>
#include <raycade/physics/PhysicsBody.hpp>
#include <raycade/physics/RigidBody.hpp>

/**
 * @brief Trafienie promieniem (Scene::Raycast()).
 */
struct RayHit {
    PhysicsBody* body = nullptr;           /**< Trafione ciało. */
    const CollisionShape* shape = nullptr; /**< Trafiony collider. */
    Vector2 point{};                       /**< Punkt trafienia w układzie sceny. */
    Vector2 normal{};                      /**< Normalna powierzchni w punkcie trafienia. */
    float distance = 0;                    /**< Odległość od początku promienia. */
};

/** @brief Maska bitowa wszystkich warstw kolizji. */
inline constexpr std::uint32_t kAllLayers = 0xFFFFFFFFu;

/**
 * @brief Świat fizyki jednej sceny (system sceny, obok PointerSystem) - liczony przez Box2D.
 * @details Scena ma jeden świat i sama go prowadzi - gra używa go pośrednio, przez ciała
 * (CharacterBody::MoveAndSlide(), Area::OverlappingBodies()) i ustawienia sceny
 * (Scene::Gravity(), Scene::DebugColliders()).
 *
 * Obiekty sceny są odwzorowane w świecie Box2D (v3, jednostki: piksele):
 * | Raycade         | Box2D                                                                |
 * |-----------------|----------------------------------------------------------------------|
 * | RigidBody       | ciało dynamiczne (masa = Mass(), rozłożona na pole colliderów)       |
 * | StaticBody      | ciało statyczne; przesunięte w kodzie - kinematyczne (ma prędkość)   |
 * | CharacterBody   | ciało kinematyczne; ruch: mover (kapsuła z obrysu colliderów)        |
 * | Area            | czujniki (sensor) - nachodzenie z b2Shape_GetSensorOverlaps()        |
 * | Collider\<S\>   | wielokąty (do 8 wierzchołków, większe dzielone), koła, odcinki;      |
 * |                 | elipsa i nierówno przeskalowane koło - przybliżenie ośmiokątem      |
 *
 * Przebieg kroku stałego sceny:
 * 1. BeginStep() - zapamiętuje krok, uzgadnia ciała i kształty Box2D z drzewem sceny,
 * 2. FixedTick() / OnFixedUpdate() obiektów - postacie ruszają się przez MoveAndSlide(),
 * 3. EndStep() - pozycje obiektów kinematycznych jako cele kroku, krok Box2D, przepisanie
 *    pozycji brył do obiektów, zdarzenia zderzeń i obszarów.
 *
 * Box2D nie jest widoczny w nagłówkach API - jego stan jest w strukturze Impl w pliku .cpp.
 * Obiekty ekranu (ScreenSpace()) nie biorą udziału w kolizjach.
 */
class PhysicsWorld {
  public:
    /** @param roots Obiekty najwyższego poziomu sceny (lista należy do sceny). */
    explicit PhysicsWorld(const std::vector<std::unique_ptr<Entity>>& roots);

    /** @brief Usuwa świat Box2D. */
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    /** @brief Ustawia grawitację w pikselach/s² (czyta ją CollisionObject::Gravity()). */
    PhysicsWorld& Gravity(Vector2 gravity);

    /** @brief Podaje grawitację. */
    Vector2 Gravity() const { return gravity_; }

    /** @brief Włącza rysowanie colliderów (Scene::DebugColliders()). */
    PhysicsWorld& Debug(bool value) {
        debug_ = value;
        return *this;
    }

    /** @brief Sprawdza, czy collidery są rysowane. */
    bool IsDebug() const { return debug_; }

    /** @brief Włącza interpolację rysowania obiektów fizyki (Scene::PhysicsInterpolation()). */
    PhysicsWorld& Interpolation(bool value) {
        interpolation_ = value;
        return *this;
    }

    /** @brief Sprawdza, czy interpolacja rysowania jest włączona. */
    bool IsInterpolation() const { return interpolation_; }

    /** @brief Ustawia ułamek drogi do następnego kroku stałego (Engine::FixedAlpha()). */
    PhysicsWorld& Alpha(float alpha) {
        alpha_ = alpha;
        return *this;
    }

    /** @brief Ułamek drogi do następnego kroku stałego - położenie rysowania między krokami. */
    float Alpha() const { return alpha_; }

    /** @brief Krok czasu dla MoveAndSlide(): krok stały albo czas klatki. */
    float Delta() const { return delta_; }

    /** @brief Początek klatki sceny - MoveAndSlide() w Tick() użyje czasu klatki. */
    void BeginFrame(float dt);

    /** @brief Początek kroku stałego. */
    void BeginStep(float step);

    /** @brief Koniec kroku stałego: krok Box2D i zdarzenia. */
    void EndStep();

    /**
     * @brief Oznacza ciała jako nieaktualne (np. po usunięciu obiektów ze sceny).
     * @details Następne użycie świata uzgodni ciała Box2D z drzewem sceny.
     */
    void Invalidate() { dirty_ = true; }

    /**
     * @brief Przesuwa postać z wypychaniem i ślizganiem (patrz CharacterBody::MoveAndSlide()).
     * @return true, gdy postać na coś wpadła.
     */
    bool MoveAndSlide(CharacterBody& body);

    /** @brief Ciała nachodzące na obszar (stan z ostatniego kroku). */
    std::vector<PhysicsBody*> OverlappingBodies(const Area& area);

    /** @brief Obszary nachodzące na obszar (stan z ostatniego kroku). */
    std::vector<Area*> OverlappingAreas(const Area& area);

    /** @brief Pierwsze ciało na odcinku from-to (patrz Scene::Raycast()). */
    std::optional<RayHit> Raycast(
        Vector2 from,
        Vector2 to,
        std::uint32_t mask,
        const CollisionObject* exclude
    ) const;

    /** @brief Ciała nachodzące na figurę w układzie sceny (patrz Scene::Overlap()). */
    std::vector<PhysicsBody*> Overlap(
        const ConvexPart& part,
        std::uint32_t mask,
        const CollisionObject* exclude
    ) const;

    /**
     * @brief Rysuje collidery wszystkich obiektów (w układzie świata, przez kamerę).
     * @details Kolory: StaticBody - niebieski, CharacterBody - zielony, RigidBody - fioletowy,
     * Area - pomarańczowy, collider wyłączony - szary. Rysowany jest kształt figury, a nie jego
     * przybliżenie w Box2D (np. ośmiokąt zamiast elipsy).
     * @param renderer Renderer prymitywów.
     */
    void Draw(PrimitiveRenderer& renderer) const;

    /**
     * @brief Zbiera collidery obiektu (potomków, którzy nie należą do innego obiektu kolizji).
     * @param object Obiekt kolizji.
     * @param shapes Lista, do której dopisywane są collidery.
     */
    static void CollectShapes(const Entity& object, std::vector<const CollisionShape*>& shapes);

  private:
    struct Impl; /**< Stan Box2D (świat, ciała, kształty) - zdefiniowany w PhysicsWorld.cpp. */

    /** @brief Para obiektów (zawsze w tej samej kolejności) - klucz zderzeń i nakładania. */
    using Pair = std::pair<const CollisionObject*, const CollisionObject*>;

    /** @brief Zderzenie zapisane dla obu ciał. */
    struct Contact {
        PhysicsBody* first = nullptr;
        PhysicsBody* second = nullptr;
        Collision forFirst;
        Collision forSecond;
    };

    /** @brief Styk pary colliderów: (obszar, jego collider, ciało, collider ciała). */
    using ShapePair = std::tuple<
        const CollisionObject*,
        const CollisionShape*,
        const CollisionObject*,
        const CollisionShape*>;

    /**
     * @brief Uzgadnia ciała i kształty Box2D z drzewem sceny (gdy dirty_).
     * @details Tworzy ciała dla nowych obiektów, usuwa je dla obiektów usuniętych ze sceny,
     * przebudowuje kształty, których geometria się zmieniła, i ustawia filtry oraz materiały.
     */
    void SyncStructure() const;

    /** @brief Przenosi do Box2D pozycje obiektów przesuniętych w kodzie (przed zapytaniami). */
    void SyncTransforms() const;

    /** @brief Zbiera obiekty kolizji z poddrzewa (owner - indeks obiektu, do którego należą). */
    void Collect(const Entity& entity, int owner) const;

    /** @brief Tworzy albo przebudowuje kształty Box2D obiektu o podanym indeksie. */
    void SyncShapes(std::size_t index) const;

    /** @brief Ustawia gęstość kształtów bryły tak, żeby jej masa była równa Mass(). */
    void ApplyMass(std::size_t index) const;

    /** @brief Przed krokiem: cele obiektów kinematycznych, parametry, siły i impulsy brył. */
    void PrepareStep();

    /** @brief Po kroku: pozycje i prędkości brył z Box2D do obiektów sceny. */
    void ReadBack();

    /** @brief Zapisuje zderzenie ciała z ciałem do zdarzeń na końcu kroku. */
    void Record(PhysicsBody& body, const Collision& hit);

    const std::vector<std::unique_ptr<Entity>>& roots_; /**< Obiekty sceny. */
    std::unique_ptr<Impl> impl_;                        /**< Świat Box2D. */
    Vector2 gravity_{0.0f, 980.0f};                     /**< Grawitacja sceny. */
    float delta_ = 0.0f;                                /**< Krok dla MoveAndSlide(). */
    bool debug_ = false;                                /**< Rysowanie colliderów. */
    bool interpolation_ = true;                         /**< Interpolacja rysowania. */
    float alpha_ = 1.0f;                                /**< Ułamek do następnego kroku. */
    mutable bool dirty_ = true;                         /**< Czy uzgodnić ciała z drzewem. */

    std::map<Pair, Contact> contacts_;      /**< Zderzenia z bieżącego kroku. */
    std::map<Pair, Contact> lastContacts_;  /**< Zderzenia z poprzedniego kroku. */
    std::set<Pair> lastOverlaps_;           /**< Pary (obszar, obiekt) z poprzedniego kroku. */
    std::set<ShapePair> lastShapeOverlaps_; /**< Styki colliderów z poprzedniego kroku. */
};
