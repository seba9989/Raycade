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
 * @brief Świat fizyki jednej sceny (system sceny, obok PointerSystem).
 * @details Scena ma jeden świat i sama go prowadzi - gra używa go pośrednio, przez ciała
 * (CharacterBody::MoveAndSlide(), Area::OverlappingBodies()) i ustawienia sceny
 * (Scene::Gravity(), Scene::DebugColliders()).
 *
 * Przebieg kroku stałego sceny:
 * 1. BeginStep() - zapamiętuje krok i oznacza listę ciał do zebrania od nowa,
 * 2. FixedTick() / OnFixedUpdate() obiektów - postacie ruszają się przez MoveAndSlide(),
 *    a świat zapisuje ich zderzenia,
 * 3. EndStep() - sprawdza obszary (Area) i wysyła zdarzenia wejścia, trwania i wyjścia.
 *
 * Lista ciał i colliderów jest zbierana z drzewa sceny na żądanie (raz na krok), a ich
 * geometria - liczona przy każdym teście, więc zmiany pozycji w trakcie kroku są widoczne od
 * razu. Obiekty ekranu (ScreenSpace()) nie biorą udziału w kolizjach.
 */
class PhysicsWorld {
  public:
    /** @param roots Obiekty najwyższego poziomu sceny (lista należy do sceny). */
    explicit PhysicsWorld(const std::vector<std::unique_ptr<Entity>>& roots) : roots_(roots) {}

    /** @brief Ustawia grawitację w pikselach/s² (czyta ją CollisionObject::Gravity()). */
    void Gravity(Vector2 gravity) { gravity_ = gravity; }

    /** @brief Podaje grawitację. */
    Vector2 Gravity() const { return gravity_; }

    /** @brief Włącza rysowanie colliderów (Scene::DebugColliders()). */
    void Debug(bool value) { debug_ = value; }

    /** @brief Sprawdza, czy collidery są rysowane. */
    bool IsDebug() const { return debug_; }

    /** @brief Włącza interpolację rysowania obiektów fizyki (Scene::PhysicsInterpolation()). */
    void Interpolation(bool value) { interpolation_ = value; }

    /** @brief Sprawdza, czy interpolacja rysowania jest włączona. */
    bool IsInterpolation() const { return interpolation_; }

    /** @brief Ustawia ułamek drogi do następnego kroku stałego (Engine::FixedAlpha()). */
    void Alpha(float alpha) { alpha_ = alpha; }

    /** @brief Ułamek drogi do następnego kroku stałego - położenie rysowania między krokami. */
    float Alpha() const { return alpha_; }

    /** @brief Krok czasu dla MoveAndSlide(): krok stały albo czas klatki. */
    float Delta() const { return delta_; }

    /** @brief Początek klatki sceny - MoveAndSlide() w Tick() użyje czasu klatki. */
    void BeginFrame(float dt);

    /** @brief Początek kroku stałego. */
    void BeginStep(float step);

    /** @brief Koniec kroku stałego: obszary i zdarzenia zderzeń. */
    void EndStep();

    /**
     * @brief Oznacza listę ciał jako nieaktualną (np. po usunięciu obiektów ze sceny).
     * @details Następne zapytanie zbierze ją z drzewa od nowa.
     */
    void Invalidate() { dirty_ = true; }

    /**
     * @brief Przesuwa postać z wypychaniem i ślizganiem (patrz CharacterBody::MoveAndSlide()).
     * @return true, gdy postać na coś wpadła.
     */
    bool MoveAndSlide(CharacterBody& body);

    /** @brief Ciała nachodzące teraz na obszar. */
    std::vector<PhysicsBody*> OverlappingBodies(const Area& area);

    /** @brief Obszary nachodzące teraz na obszar. */
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
     * @details Kolory: StaticBody - niebieski, CharacterBody - zielony, Area - pomarańczowy,
     * collider wyłączony - szary.
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
    /** @brief Obiekt kolizji z jego colliderami, zebrany z drzewa sceny. */
    struct Entry {
        CollisionObject* object = nullptr;
        PhysicsBody* body = nullptr; /**< Ten sam obiekt, jeśli jest ciałem. */
        Area* area = nullptr;        /**< Ten sam obiekt, jeśli jest obszarem. */
        RigidBody* rigid = nullptr;  /**< Ten sam obiekt, jeśli jest bryłą sztywną. */
        std::vector<const CollisionShape*> shapes;
    };

    /** @brief Para obiektów (zawsze w tej samej kolejności) - klucz zderzeń i nakładania. */
    using Pair = std::pair<const CollisionObject*, const CollisionObject*>;

    /** @brief Zderzenie zapisane dla obu ciał. */
    struct Contact {
        PhysicsBody* first = nullptr;
        PhysicsBody* second = nullptr;
        Collision forFirst;
        Collision forSecond;
    };

    /** @brief Aktualna lista obiektów kolizji (zbierana od nowa, gdy jest nieaktualna). */
    const std::vector<Entry>& Entries() const;

    /** @brief Zbiera obiekty kolizji z poddrzewa (owner - indeks obiektu, do którego należą). */
    void Collect(const Entity& entity, int owner) const;

    /**
     * @brief Najgłębsze wnikanie postaci w ciała, z którymi się zderza.
     * @param self Postać.
     * @param motion Ostatni ruch postaci - platforma jednostronna zatrzymuje tylko ruch w dół.
     */
    std::optional<Collision> Deepest(const Entry& self, Vector2 motion) const;

    /** @brief Sprawdza, czy dwa obiekty kolizji nachodzą na siebie którymkolwiek colliderem. */
    static bool Overlap(const Entry& a, const Entry& b);

    /** @brief Wpis obiektu albo nullptr. */
    const Entry* Find(const CollisionObject* object) const;

    /** @brief Zapisuje zderzenie postaci z ciałem do zdarzeń na końcu kroku. */
    void Record(PhysicsBody& body, const Collision& hit);

    /**
     * @brief Krok brył sztywnych (solver impulsowy w stylu Box2D).
     * @details 1. siły, impulsy, grawitacja i tłumienie zmieniają prędkości, 2. punkty styku
     * (ConvexContacts()) z impulsami z poprzedniego kroku (warm starting), 3. kilka iteracji
     * impulsów normalnych (z odbiciem i korektą wnikania) i tarcia, 4. przesunięcie i obrót.
     * @param dt Krok w sekundach.
     */
    void Simulate(float dt);

    /** @brief Impulsy styku z poprzedniego kroku - start solvera (warm starting). */
    struct CachedContact {
        Vector2 point{};
        float normalImpulse = 0;
        float tangentImpulse = 0;
    };
    /** @brief Para colliderów brył: (bryła, jej collider, drugie ciało, jego collider). */
    using ManifoldKey = std::tuple<
        const CollisionObject*,
        const CollisionShape*,
        const CollisionObject*,
        const CollisionShape*>;
    std::map<ManifoldKey, std::vector<CachedContact>> warmStart_; /**< Impulsy styków. */

    const std::vector<std::unique_ptr<Entity>>& roots_; /**< Obiekty sceny. */
    Vector2 gravity_{0.0f, 980.0f};                     /**< Grawitacja sceny. */
    float delta_ = 0.0f;                                /**< Krok dla MoveAndSlide(). */
    bool debug_ = false;                                /**< Rysowanie colliderów. */
    bool interpolation_ = true;                         /**< Interpolacja rysowania. */
    float alpha_ = 1.0f;                                /**< Ułamek do następnego kroku. */

    mutable std::vector<Entry> entries_; /**< Obiekty kolizji (zbierane na żądanie). */
    mutable bool dirty_ = true;          /**< Czy entries_ trzeba zebrać od nowa. */

    std::map<Pair, Contact> contacts_;     /**< Zderzenia z bieżącego kroku. */
    std::map<Pair, Contact> lastContacts_; /**< Zderzenia z poprzedniego kroku. */
    std::set<Pair> lastOverlaps_;          /**< Pary (obszar, obiekt) z poprzedniego kroku. */

    /** @brief Styk pary colliderów: (obszar, jego collider, ciało, collider ciała). */
    using ShapePair = std::tuple<
        const CollisionObject*,
        const CollisionShape*,
        const CollisionObject*,
        const CollisionShape*>;
    std::set<ShapePair> lastShapeOverlaps_; /**< Styki colliderów z poprzedniego kroku. */
};
