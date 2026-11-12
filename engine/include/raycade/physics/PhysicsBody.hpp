/**
 * @file PhysicsBody.hpp
 * @brief Klasa PhysicsBody i struktura Collision - ciała, które się zderzają.
 */
#pragma once

#include <functional>
#include <utility>

#include <raycade/physics/CollisionObject.hpp>

class PhysicsBody;

/**
 * @brief Zderzenie dwóch ciał widziane z perspektywy jednego z nich.
 * @details Wskaźniki są ważne w trakcie zdarzenia (OnCollisionEnter(), OnCollisionStay())
 * i do następnego CharacterBody::MoveAndSlide() - nie należy ich przechowywać dłużej.
 */
struct Collision {
    PhysicsBody* other = nullptr;               /**< Drugie ciało. */
    const CollisionShape* shape = nullptr;      /**< Mój collider, który się zderzył. */
    const CollisionShape* otherShape = nullptr; /**< Collider drugiego ciała. */
    Vector2 normal{};                           /**< Kierunek od drugiego ciała do mnie. */
    float depth = 0.0f;                         /**< Głębokość wnikania przed rozsunięciem. */
};

/**
 * @brief Ciało, które zderza się z innymi ciałami (baza StaticBody i CharacterBody).
 * @details Zdarzenia zderzeń (jak OnCollisionEnter2D w Unity) dostają oba ciała zderzenia.
 * Zderzenie powstaje, gdy CharacterBody::MoveAndSlide() wpada na inne ciało - zdarzenia są
 * wysyłane na końcu kroku stałego, do obu ciał:
 * - OnCollisionEnter - pierwszy krok, w którym ciała się zderzyły,
 * - OnCollisionStay - każdy kolejny krok zderzenia (np. postać stoi na podłodze),
 * - OnCollisionExit - pierwszy krok bez zderzenia.
 * @code{.cpp}
 * scene.Spawn<StaticBody>().OnCollisionEnter([](StaticBody& self, const Collision& hit) {
 *     self.Destroy(); // np. kruszący się blok
 * });
 * @endcode
 */
class PhysicsBody : public CollisionObject {
  public:
    /**
     * @brief Ustawia tarcie powierzchni (dla brył sztywnych - RigidBody).
     * @details Dla pary ciał używana jest średnia geometryczna ich tarć: 0 - lód, 1 - guma.
     * @param friction Współczynnik tarcia (domyślnie 0.6).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Friction(this Self& self, float friction) {
        self.friction_ = friction;
        return self;
    }

    /** @brief Podaje współczynnik tarcia. */
    float Friction() const { return friction_; }

    /**
     * @brief Ustawia sprężystość odbicia (dla brył sztywnych - RigidBody).
     * @details Dla pary ciał używana jest większa z wartości: 0 - bez odbicia, 1 - odbicie
     * bez straty prędkości.
     * @param bounce Współczynnik restytucji (domyślnie 0).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Bounce(this Self& self, float bounce) {
        self.bounce_ = bounce;
        return self;
    }

    /** @brief Podaje współczynnik odbicia. */
    float Bounce() const { return bounce_; }

    /**
     * @brief Ustawia reakcję na początek zderzenia z innym ciałem.
     * @param callback Funkcja otrzymująca ciało i opis zderzenia.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& OnCollisionEnter(this Self& self, Callback<Self, const Collision&> callback) {
        self.enter_ = [&self, fn = std::move(callback)](const Collision& hit) { fn(self, hit); };
        return self;
    }

    /**
     * @brief Ustawia reakcję na trwające zderzenie (każdy kolejny krok stały).
     * @param callback Funkcja otrzymująca ciało i opis zderzenia.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& OnCollisionStay(this Self& self, Callback<Self, const Collision&> callback) {
        self.stay_ = [&self, fn = std::move(callback)](const Collision& hit) { fn(self, hit); };
        return self;
    }

    /**
     * @brief Ustawia reakcję na koniec zderzenia z innym ciałem.
     * @param callback Funkcja otrzymująca ciało i drugie ciało.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& OnCollisionExit(this Self& self, Callback<Self, PhysicsBody&> callback) {
        self.exit_ = [&self, fn = std::move(callback)](PhysicsBody& other) { fn(self, other); };
        return self;
    }

  private:
    friend class PhysicsWorld;

    std::function<void(const Collision&)> enter_; /**< Początek zderzenia. */
    std::function<void(const Collision&)> stay_;  /**< Trwające zderzenie. */
    std::function<void(PhysicsBody&)> exit_;      /**< Koniec zderzenia. */
    float friction_ = 0.6f;                       /**< Tarcie powierzchni. */
    float bounce_ = 0.0f;                         /**< Sprężystość odbicia. */
};

/**
 * @brief Nieruchome ciało: ściana, podłoga, platforma (StaticBody2D z Godota).
 * @details Inne ciała odbijają się od niego, a ono samo nie jest przesuwane przy zderzeniach.
 * Można je przesuwać w kodzie (Position(), Translate()) - np. ruchoma platforma.
 */
class StaticBody : public PhysicsBody {};
