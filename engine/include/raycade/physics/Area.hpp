/**
 * @file Area.hpp
 * @brief Klasa Area - obszar wykrywający ciała i inne obszary (bez fizyki).
 */
#pragma once

#include <functional>
#include <utility>
#include <vector>

#include <raycade/physics/PhysicsBody.hpp>

/**
 * @brief Obszar, który wykrywa wejście i wyjście ciał, ale ich nie zatrzymuje (Area2D z Godota).
 * @details Przydaje się do monet, kolców, stref śmierci, drzwi i wyzwalaczy. Zdarzenia są
 * wysyłane na końcu kroku stałego. Area wykrywa obiekty, których warstwy pasują do jej
 * CollisionMask().
 * @code{.cpp}
 * scene.Spawn<Area>()
 *     .Position({500, 600})
 *     .Children([](Area& coin) {
 *         coin.Spawn<collider::Circle>().Radius(10).Paint(GOLD).Fill().Visible();
 *     })
 *     .OnBodyEnter([](Area& coin, PhysicsBody& body) {
 *         if (dynamic_cast<Player*>(&body)) coin.Destroy();
 *     });
 * @endcode
 */
class Area : public CollisionObject {
  public:
    /**
     * @brief Ustawia reakcję na wejście ciała w obszar.
     * @param callback Funkcja otrzymująca obszar i ciało.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& OnBodyEnter(this Self& self, Callback<Self, PhysicsBody&> callback) {
        self.bodyEnter_ = [&self, fn = std::move(callback)](PhysicsBody& body) { fn(self, body); };
        return self;
    }

    /**
     * @brief Ustawia reakcję na wyjście ciała z obszaru (także gdy ciało zostało usunięte).
     * @param callback Funkcja otrzymująca obszar i ciało.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& OnBodyExit(this Self& self, Callback<Self, PhysicsBody&> callback) {
        self.bodyExit_ = [&self, fn = std::move(callback)](PhysicsBody& body) { fn(self, body); };
        return self;
    }

    /**
     * @brief Ustawia reakcję na wejście innego obszaru.
     * @param callback Funkcja otrzymująca ten obszar i drugi.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnAreaEnter(this Self& self, Callback<Self, Area&> callback) {
        self.areaEnter_ = [&self, fn = std::move(callback)](Area& area) { fn(self, area); };
        return self;
    }

    /**
     * @brief Ustawia reakcję na wyjście innego obszaru.
     * @param callback Funkcja otrzymująca ten obszar i drugi.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnAreaExit(this Self& self, Callback<Self, Area&> callback) {
        self.areaExit_ = [&self, fn = std::move(callback)](Area& area) { fn(self, area); };
        return self;
    }

    /** @brief Ciała, które teraz są w obszarze (liczone przy wywołaniu). */
    std::vector<PhysicsBody*> OverlappingBodies() const;

    /** @brief Inne obszary, które teraz nachodzą na ten obszar (liczone przy wywołaniu). */
    std::vector<Area*> OverlappingAreas() const;

  private:
    friend class PhysicsWorld;

    std::function<void(PhysicsBody&)> bodyEnter_; /**< Wejście ciała. */
    std::function<void(PhysicsBody&)> bodyExit_;  /**< Wyjście ciała. */
    std::function<void(Area&)> areaEnter_;        /**< Wejście obszaru. */
    std::function<void(Area&)> areaExit_;         /**< Wyjście obszaru. */
};
