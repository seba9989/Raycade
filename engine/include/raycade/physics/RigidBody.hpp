/**
 * @file RigidBody.hpp
 * @brief Klasa RigidBody - bryła sztywna poruszana przez fizykę (masa, odbicia, tarcie, obrót).
 */
#pragma once

#include <vector>

#include <raycade/physics/PhysicsBody.hpp>

/**
 * @brief Bryła sztywna: ruch i obrót liczy fizyka (RigidBody2D z Godota, Rigidbody2D z Unity).
 * @details Ciało spada pod wpływem grawitacji, odbija się (Bounce()), trze o powierzchnie
 * (Friction()) i obraca od uderzeń. Masę rozkłada równomiernie na pole colliderów: środek masy
 * i moment bezwładności wynikają z ich kształtu, więc długa deska obraca się wolniej niż kulka.
 *
 * Z innymi ciałami:
 * - StaticBody i CharacterBody mają dla brył nieskończoną masę - bryła się od nich odbija, ale ich
 *   nie przesuwa; postać pcha skrzynię sama, np. impulsem z SlideCollisions(),
 * - dwie bryły odpychają się zgodnie z masami.
 *
 * Fizyka działa w kroku stałym, po FixedTick() wszystkich obiektów. Ruchem steruje się siłami
 * i impulsami albo wprost prędkością; Position() i Rotate() przenoszą bryłę natychmiast.
 * @code{.cpp}
 * auto& crate = scene.Spawn<RigidBody>().Position({400, 100}).Mass(2).Friction(0.8f);
 * crate.Spawn<collider::Rect>().Position({-15, -15}).Size({30,
 * 30}).Paint(BROWN).Filled().Visible();
 *
 * auto& ball = scene.Spawn<RigidBody>().Position({500, 100}).Bounce(0.7f);
 * ball.Spawn<collider::Circle>().Radius(12).Paint(RED).Filled().Visible();
 * ball.ApplyCentralImpulse({200, -100}); // rzut
 * @endcode
 */
class RigidBody : public PhysicsBody {
  public:
    /**
     * @brief Ustawia masę (rozłożoną równomiernie na pole colliderów).
     * @param mass Masa w dowolnych jednostkach (domyślnie 1); liczy się stosunek mas brył.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Mass(this Self& self, float mass) {
        self.mass_ = mass > 0.0f ? mass : 0.001f;
        return self;
    }

    /** @brief Podaje masę. */
    float Mass() const { return mass_; }

    /**
     * @brief Ustawia mnożnik grawitacji sceny (0 - bryła nie spada, np. w kosmosie).
     * @param scale Mnożnik (domyślnie 1).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& GravityScale(this Self& self, float scale) {
        self.gravityScale_ = scale;
        return self;
    }

    /** @brief Podaje mnożnik grawitacji. */
    float GravityScale() const { return gravityScale_; }

    /**
     * @brief Ustawia prędkość środka masy w px/s (układ sceny).
     * @param velocity Prędkość.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Velocity(this Self& self, Vector2 velocity) {
        self.velocity_ = velocity;
        return self;
    }

    /** @brief Podaje prędkość środka masy. */
    Vector2 Velocity() const { return velocity_; }

    /**
     * @brief Ustawia prędkość obrotu w radianach na sekundę (dodatnia - zgodnie z Rotate()).
     * @param velocity Prędkość kątowa.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& AngularVelocity(this Self& self, float velocity) {
        self.angularVelocity_ = velocity;
        return self;
    }

    /** @brief Podaje prędkość kątową. */
    float AngularVelocity() const { return angularVelocity_; }

    /**
     * @brief Ustawia tłumienie ruchu (opór powietrza): ułamek prędkości traconej na sekundę.
     * @param damp Tłumienie (domyślnie 0).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& LinearDamp(this Self& self, float damp) {
        self.linearDamp_ = damp;
        return self;
    }

    /** @brief Podaje tłumienie ruchu. */
    float LinearDamp() const { return linearDamp_; }

    /**
     * @brief Ustawia tłumienie obrotu: ułamek prędkości kątowej traconej na sekundę.
     * @param damp Tłumienie (domyślnie 0).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& AngularDamp(this Self& self, float damp) {
        self.angularDamp_ = damp;
        return self;
    }

    /** @brief Podaje tłumienie obrotu. */
    float AngularDamp() const { return angularDamp_; }

    /**
     * @brief Blokuje obrót - bryła tylko się przesuwa (np. postać sterowana fizyką).
     * @param value Czy zablokować.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& RotationLocked(this Self& self, bool value = true) {
        self.lockRotation_ = value;
        return self;
    }

    /** @brief Sprawdza, czy obrót jest zablokowany. */
    bool IsRotationLocked() const { return lockRotation_; }

    /**
     * @brief Natychmiastowa zmiana pędu przyłożona w środku masy (np. skok, wybuch, rzut).
     * @details Zmienia prędkość o impulse / Mass() na początku najbliższego kroku stałego.
     * @param impulse Impuls w układzie sceny.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& ApplyCentralImpulse(this Self& self, Vector2 impulse) {
        self.pending_.push_back({impulse, {}, true, true});
        return self;
    }

    /**
     * @brief Impuls przyłożony w punkcie - poza środkiem masy także obraca bryłę (np. strzał).
     * @param impulse Impuls w układzie sceny.
     * @param point Punkt przyłożenia w układzie sceny.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& ApplyImpulse(this Self& self, Vector2 impulse, Vector2 point) {
        self.pending_.push_back({impulse, point, true, false});
        return self;
    }

    /**
     * @brief Siła przyłożona w środku masy przez jeden krok stały (np. silnik, wiatr).
     * @details Siła działa tylko w najbliższym kroku - stałą siłę przykłada się co krok
     * (w FixedTick() albo OnFixedUpdate()).
     * @param force Siła w układzie sceny.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& ApplyCentralForce(this Self& self, Vector2 force) {
        self.pending_.push_back({force, {}, false, true});
        return self;
    }

    /**
     * @brief Siła przyłożona w punkcie przez jeden krok stały - poza środkiem masy także obraca.
     * @param force Siła w układzie sceny.
     * @param point Punkt przyłożenia w układzie sceny.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& ApplyForce(this Self& self, Vector2 force, Vector2 point) {
        self.pending_.push_back({force, point, false, false});
        return self;
    }

    /**
     * @brief Moment siły przez jeden krok stały (obraca bez przesuwania).
     * @param torque Moment (dodatni - zgodnie z Rotate()).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& ApplyTorque(this Self& self, float torque) {
        self.torque_ += torque;
        return self;
    }

    /**
     * @brief Natychmiastowa zmiana momentu pędu (obraca bez przesuwania).
     * @param impulse Impuls kątowy.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& ApplyTorqueImpulse(this Self& self, float impulse) {
        self.torqueImpulse_ += impulse;
        return self;
    }

    /** @brief Środek masy w układzie sceny (z ostatniego kroku fizyki). */
    Vector2 CenterOfMass() const { return centerOfMass_; }

  private:
    friend class PhysicsWorld;

    /** @brief Siła albo impuls czekający na najbliższy krok. */
    struct Pending {
        Vector2 vector{}; /**< Siła albo impuls. */
        Vector2 point{};  /**< Punkt przyłożenia (gdy nie central). */
        bool impulse = false;
        bool central = false;
    };

    float mass_ = 1.0f;            /**< Masa. */
    float gravityScale_ = 1.0f;    /**< Mnożnik grawitacji. */
    Vector2 velocity_{};           /**< Prędkość środka masy. */
    float angularVelocity_ = 0;    /**< Prędkość kątowa. */
    float linearDamp_ = 0;         /**< Tłumienie ruchu. */
    float angularDamp_ = 0;        /**< Tłumienie obrotu. */
    bool lockRotation_ = false;    /**< Blokada obrotu. */
    std::vector<Pending> pending_; /**< Siły i impulsy do najbliższego kroku. */
    float torque_ = 0;             /**< Moment siły do najbliższego kroku. */
    float torqueImpulse_ = 0;      /**< Impuls kątowy do najbliższego kroku. */
    Vector2 centerOfMass_{};       /**< Środek masy z ostatniego kroku. */
};
