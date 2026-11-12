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
 * sceny (WorldParts()) i o obiekt, do którego należy (Object()). Ustawienia Disabled() i
 * Visible() są wspólne dla colliderów o dowolnym kształcie.
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
     * @brief Rysuje collider jak zwykłą figurę i pozwala go klikać.
     * @details Domyślnie collider jest niewidoczny i nie reaguje na mysz - jest tylko kształtem
     * kolizji. Widoczny collider to grafika i kolizja w jednym obiekcie, np. platforma.
     * @param value Czy rysować.
     * @return Ten sam collider.
     */
    template <typename Self> Self& Visible(this Self& self, bool value = true) {
        self.visible_ = value;
        return self;
    }

    /** @brief Sprawdza, czy collider jest rysowany (Visible()). */
    bool IsVisible() const { return visible_; }

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
    bool visible_ = false;  /**< Czy collider jest rysowany. */
};
