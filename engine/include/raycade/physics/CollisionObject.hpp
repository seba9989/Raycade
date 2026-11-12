/**
 * @file CollisionObject.hpp
 * @brief Klasa CollisionObject - wspólna baza ciał i obszarów (Area).
 */
#pragma once

#include <cstdint>
#include <vector>

#include <raycade/entity/Entity.hpp>
#include <raycade/physics/CollisionShape.hpp>

/**
 * @brief Obiekt biorący udział w kolizjach: ma collidery-dzieci, warstwę i maskę.
 * @details Baza StaticBody, CharacterBody i Area (odpowiednik CollisionObject2D z Godota).
 * Sam nic nie rysuje - widać tylko jego dzieci.
 *
 * Warstwy i maski to maski bitowe (do 32 warstw). CollisionLayer() mówi, na których warstwach
 * obiekt jest, a CollisionMask() - z którymi warstwami się zderza (albo które wykrywa Area).
 * Obiekt A zderza się z B, gdy `A.CollisionMask() & B.CollisionLayer()` jest niezerowe.
 * Domyślnie oba są równe 1, więc wszystko zderza się ze wszystkim.
 * @code{.cpp}
 * enum Layers : std::uint32_t { World = 1 << 0, Player = 1 << 1, Coins = 1 << 2 };
 * player.CollisionLayer(Player).CollisionMask(World);     // gracz zderza się tylko ze światem
 * coin.CollisionLayer(Coins).CollisionMask(Player);       // moneta wykrywa tylko gracza
 * @endcode
 */
class CollisionObject : public Entity {
  public:
    /**
     * @brief Ustawia warstwy, na których jest obiekt.
     * @param layer Maska bitowa warstw.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& CollisionLayer(this Self& self, std::uint32_t layer) {
        self.layer_ = layer;
        return self;
    }

    /** @brief Podaje maskę warstw obiektu. */
    std::uint32_t CollisionLayer() const { return layer_; }

    /**
     * @brief Ustawia warstwy, z którymi obiekt się zderza (albo które wykrywa).
     * @param mask Maska bitowa warstw.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& CollisionMask(this Self& self, std::uint32_t mask) {
        self.mask_ = mask;
        return self;
    }

    /** @brief Podaje maskę warstw, z którymi obiekt się zderza. */
    std::uint32_t CollisionMask() const { return mask_; }

    /**
     * @brief Sprawdza, czy ten obiekt zderza się z innym (maska tego, warstwa drugiego).
     * @param other Drugi obiekt.
     */
    bool CollidesWith(const CollisionObject& other) const { return (mask_ & other.layer_) != 0; }

    /**
     * @brief Collidery obiektu: dzieci i dalsi potomkowie, którzy nie należą do innego obiektu.
     * @return Collidery, także wyłączone (IsDisabled()).
     */
    std::vector<const CollisionShape*> Shapes() const;

    /**
     * @brief Grawitacja sceny (Scene::Gravity()), domyślnie {0, 980} pikseli/s².
     * @details Silnik nie dodaje jej sam do CharacterBody - postać dodaje ją do Velocity(),
     * tak jak w Godocie.
     */
    Vector2 Gravity() const;

  protected:
    /** @brief Obiekt kolizji nie rysuje niczego - widać tylko jego dzieci. */
    void Draw(PrimitiveRenderer&) const override {}

    /**
     * @brief Położenie rysowania między dwoma krokami fizyki (interpolacja).
     * @details Fizyka zmienia pozycje w kroku stałym (np. 60 razy na sekundę), a obraz rysuje się
     * częściej - bez interpolacji obiekt stałby przez kilka klatek i przeskakiwał. Obiekt jest
     * więc rysowany w położeniu pośrednim między stanem przed i po ostatnim kroku
     * (Engine::FixedAlpha()), czyli do jednego kroku „w tyle”. Obiekt przesunięty poza krokiem
     * stałym (np. w OnUpdate()) albo o więcej niż 64 px naraz (teleport) jest rysowany tam,
     * gdzie jest. Wyłącza się ją dla sceny: Scene::PhysicsInterpolation(false).
     */
    AffineMatrix2D DrawTransform() const override;

    /**
     * @brief Przesuwa obiekt o wektor w układzie sceny.
     * @details Position() jest w układzie rodzica - przesunięcie jest przeliczane przez rodzica,
     * więc działa także w obróconej albo przeskalowanej grupie.
     * @param delta Przesunięcie w układzie sceny.
     */
    void MoveInScene(Vector2 delta);

    std::uint32_t layer_ = 1; /**< Warstwy obiektu. */
    std::uint32_t mask_ = 1;  /**< Warstwy, z którymi obiekt się zderza. */

  private:
    friend class PhysicsWorld;

    AffineMatrix2D previous_{}; /**< Przekształcenie przed ostatnim krokiem stałym. */
    AffineMatrix2D stepped_{};  /**< Przekształcenie po ostatnim kroku stałym. */
    bool inStep_ = false;       /**< Czy obiekt był na liście na początku kroku. */
    bool interpolated_ = false; /**< Czy previous_ i stepped_ opisują ostatni krok. */
};
