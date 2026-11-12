/**
 * @file CollisionShape.hpp
 * @brief Klasa CollisionShape - wspólna część wszystkich colliderów (Collider\<S\>).
 */
#pragma once

#include <vector>

#include <raycade/entity/Entity.hpp>
#include <raycade/math/Convex.hpp>

class CollisionObject;

/**
 * @brief Wspólna, nieszablonowa część colliderów - patrz Collider.
 * @details Świat fizyki widzi każdy collider przez tę klasę: pyta o jego geometrię w układzie
 * sceny (WorldParts()) i o obiekt, do którego należy (Object()). Ustawienia Disabled()
 * i OneWay() są wspólne dla colliderów o dowolnym kształcie; widoczność (Visible()) mają
 * wszystkie obiekty sceny.
 */
class CollisionShape {
  public:
    virtual ~CollisionShape() = default;

    /**
     * @brief Wyłącza collider - świat fizyki go pomija.
     * @param value Czy wyłączyć.
     * @return Ten sam collider.
     */
    template <typename Self> Self& Disabled(this Self& self, bool value = true) {
        self.disabled_ = value;
        return self;
    }

    /** @brief Sprawdza, czy collider jest wyłączony (Disabled()). */
    bool IsDisabled() const { return disabled_; }

    /**
     * @brief Zamienia collider w platformę jednostronną (one-way collision z Godota).
     * @details Postać przechodzi przez nią od dołu i z boków, a zatrzymuje się tylko, gdy opada
     * na nią od góry. „Góra” to lokalna oś -y collidera, więc obrócona platforma przepuszcza
     * w obróconym kierunku (OneWayDirection()). Obszary (Area) wykrywają ją normalnie, a promień
     * (Scene::Raycast()) trafia ją tylko od strony „góry”. Żeby postać zeskoczyła w dół przez
     * platformę, wystarczy na chwilę wyłączyć collider (Disabled()) albo zmienić maskę postaci.
     * @code{.cpp}
     * scene.Spawn<StaticBody>().Position({300, 400}).Children([](StaticBody& ledge) {
     *     ledge.Spawn<collider::Rect>().Size({120, 10}).OneWay().Paint(BROWN).Filled().Visible();
     * });
     * @endcode
     * @param value Czy collider jest jednostronny.
     * @return Ten sam collider.
     */
    template <typename Self> Self& OneWay(this Self& self, bool value = true) {
        self.oneWay_ = value;
        return self;
    }

    /** @brief Sprawdza, czy collider jest platformą jednostronną (OneWay()). */
    bool IsOneWay() const { return oneWay_; }

    /**
     * @brief Kierunek „w górę” platformy jednostronnej w układzie sceny (wektor jednostkowy).
     * @details Lokalna oś -y collidera po obrocie collidera i jego przodków.
     */
    virtual Vector2 OneWayDirection() const = 0;

    /**
     * @brief Obiekt kolizji (ciało albo Area), do którego należy collider.
     * @return Najbliższy przodek typu CollisionObject albo nullptr - collider bez takiego
     * przodka nie bierze udziału w kolizjach.
     */
    CollisionObject* Object() const;

    /** @brief Collider jako obiekt sceny. */
    virtual const Entity& Node() const = 0;

    /**
     * @brief Dopisuje geometrię collidera w układzie sceny (po wszystkich przekształceniach).
     * @param parts Lista, do której collider dopisuje swoje części wypukłe.
     */
    virtual void WorldParts(std::vector<ConvexPart>& parts) const = 0;

  protected:
    bool disabled_ = false; /**< Czy collider jest wyłączony. */
    bool oneWay_ = false;   /**< Czy collider jest platformą jednostronną. */
};
