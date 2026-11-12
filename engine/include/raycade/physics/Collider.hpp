/**
 * @file Collider.hpp
 * @brief Szablon Collider\<S\> - kształt kolizji o kształcie dowolnej figury z @ref shapes.
 */
#pragma once

#include <concepts>
#include <vector>

#include <raycade/physics/CollisionShape.hpp>
#include <raycade/shapes.hpp>

/**
 * @brief Kształt kolizji o kształcie figury S (odpowiednik CollisionShape2D z Godota).
 * @details Collider dziedziczy po figurze, więc ma wszystkie jej settery: `Size()` prostokąta,
 * `Radius()` okręgu, `Vertices()` wielokąta, a także Position(), Rotate() i Scale(). Kolizja ma
 * zawsze dokładnie ten kształt, w tym obrót i skalę obiektu oraz jego przodków.
 *
 * Collider sam nic nie robi - należy do najbliższego przodka, który jest obiektem kolizji
 * (StaticBody, CharacterBody albo Area). Ciało może mieć kilka colliderów, które razem tworzą
 * kształt złożony. Domyślnie collider jest niewidoczny i nie reaguje na mysz; Visible() rysuje
 * go jak zwykłą figurę.
 *
 * Krótsze nazwy są w przestrzeni nazw @ref collider (`collider::Rect` to
 * `Collider<shapes::Rect>`).
 * @code{.cpp}
 * class Player : public CharacterBody {
 *   public:
 *     Player() {
 *         Spawn<shapes::Rect>().Position({-12, -32}).Size({24, 32}).Paint(BLUE).Fill(); // grafika
 *         Spawn<collider::Rect>().Position({-12, -32}).Size({24, 32});                  // kolizja
 *     }
 * };
 *
 * // platforma: widoczny collider to grafika i kolizja naraz
 * scene.Spawn<StaticBody>().Position({0, 650}).Children([](StaticBody& floor) {
 *     floor.Spawn<collider::Rect>().Size({1000, 50}).Paint(DARKGRAY).Fill().Visible();
 * });
 * @endcode
 * @tparam S Figura z @ref shapes (dziedzicząca po shapes::Shape).
 */
template <typename S>
    requires std::derived_from<S, shapes::Shape>
class Collider : public S, public CollisionShape {
  public:
    /** @brief Niewidoczny collider nie zasłania kliknięć - trafia tylko widoczny. */
    bool Contains(Vector2 point) const override { return visible_ && S::Contains(point); }

    /** @copydoc CollisionShape::Node */
    const Entity& Node() const override { return *this; }

    /** @copydoc CollisionShape::WorldParts */
    void WorldParts(std::vector<ConvexPart>& parts) const override {
        const std::size_t first = parts.size();
        this->ConvexParts(parts);
        const AffineMatrix2D world = this->WorldTransform();
        for (std::size_t i = first; i < parts.size(); ++i)
            parts[i] = parts[i].Transformed(world);
    }

  protected:
    /** @brief Rysuje figurę tylko wtedy, gdy collider jest widoczny (Visible()). */
    void Draw(PrimitiveRenderer& renderer) const override {
        if (visible_) S::Draw(renderer);
    }
};

/**
 * @namespace collider
 * @brief Krótsze nazwy colliderów: `collider::Rect` to `Collider<shapes::Rect>` itd.
 */
namespace collider {
using Point = Collider<shapes::Point>;       /**< Collider w kształcie punktu. */
using Points = Collider<shapes::Points>;     /**< Collider z kilku punktów. */
using Line = Collider<shapes::Line>;         /**< Collider w kształcie odcinka. */
using Lines = Collider<shapes::Lines>;       /**< Collider z niezależnych odcinków. */
using Polyline = Collider<shapes::Polyline>; /**< Collider w kształcie łamanej. */
using Rect = Collider<shapes::Rect>;         /**< Collider w kształcie prostokąta. */
using Triangle = Collider<shapes::Triangle>; /**< Collider w kształcie trójkąta. */
using Polygon = Collider<shapes::Polygon>;   /**< Collider w kształcie wielokąta. */
using Circle = Collider<shapes::Circle>;     /**< Collider w kształcie koła. */
using Ellipse = Collider<shapes::Ellipse>;   /**< Collider w kształcie elipsy. */
} // namespace collider
