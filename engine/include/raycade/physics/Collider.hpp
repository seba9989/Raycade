/**
 * @file Collider.hpp
 * @brief Szablon Collider\<S\> - kształt kolizji o kształcie dowolnej figury z @ref shapes.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <vector>

#include <raycade/physics/CollisionShape.hpp>
#include <raycade/shapes.hpp>
#include <raycade/visual/Sprite.hpp>

/**
 * @brief Kształt kolizji o kształcie figury S (odpowiednik CollisionShape2D z Godota).
 * @details Collider dziedziczy po figurze, więc ma wszystkie jej settery: `Size()` prostokąta,
 * `Radius()` okręgu, `Vertices()` wielokąta, a także Position(), Rotate() i Scale(). Kolizja ma
 * zawsze dokładnie ten kształt, w tym obrót i skalę obiektu oraz jego przodków.
 *
 * Collider sam nic nie robi - należy do najbliższego przodka, który jest obiektem kolizji
 * (StaticBody, CharacterBody albo Area). Ciało może mieć kilka colliderów, które razem tworzą
 * kształt złożony. W odróżnieniu od innych obiektów collider jest domyślnie niewidoczny
 * i nie reaguje na mysz; Visible() (Drawable::Visible()) rysuje go jak zwykłą figurę.
 *
 * Krótsze nazwy są w przestrzeni nazw @ref collider (`collider::Rect` to
 * `Collider<shapes::Rect>`).
 * @code{.cpp}
 * class Player : public CharacterBody {
 *   public:
 *     Player() {
 *         // grafika
 *         Spawn<shapes::Rect>().Position({-12, -32}).Size({24, 32}).Paint(BLUE).Filled();
 *         // kolizja
 *         Spawn<collider::Rect>().Position({-12, -32}).Size({24, 32});
 *     }
 * };
 *
 * // platforma: widoczny collider to grafika i kolizja naraz
 * scene.Spawn<StaticBody>().Position({0, 650}).Children([](StaticBody& floor) {
 *     floor.Spawn<collider::Rect>().Size({1000, 50}).Paint(DARKGRAY).Filled().Visible();
 * });
 * @endcode
 * @tparam S Figura z @ref shapes (dziedzicząca po shapes::Shape).
 */
template <typename S>
    requires std::derived_from<S, shapes::Shape>
class Collider : public S, public CollisionShape {
  public:
    /** @brief Tworzy niewidoczny collider (sam kształt kolizji). */
    Collider() { this->visible_ = false; }

    /** @brief Niewidoczny collider nie zasłania kliknięć - trafia tylko widoczny. */
    bool Contains(Vector2 point) const override { return this->visible_ && S::Contains(point); }

    /** @copydoc CollisionShape::Node */
    const Entity& Node() const override { return *this; }

    /** @copydoc CollisionShape::OneWayDirection */
    Vector2 OneWayDirection() const override {
        // oś -y po części liniowej przekształcenia (bez przesunięcia)
        const AffineMatrix2D world = this->WorldTransform();
        const Vector2 up{-world.c, -world.d};
        const float length = std::sqrt(up.x * up.x + up.y * up.y);
        return length > 0.0f ? Vector2{up.x / length, up.y / length} : Vector2{0.0f, -1.0f};
    }

    /**
     * @brief Dopasowuje prostokątny collider do bieżącej klatki sprite'a.
     * @details Ustawia Position() i Size() tak, żeby collider pokrywał prostokąt klatki
     * (FrameSize()) z uwzględnieniem Pivot() (Origin() i Anchor()) sprite'a. Sprite musi mieć tego
     * samego rodzica co collider (zwykle oba są dziećmi ciała). Wymiary są kopiowane raz - collider
     * nie śledzi późniejszych zmian klatki ani obrotu i skali samego sprite'a.
     * @code{.cpp}
     * auto& hero = Spawn<visual::Sprite>().Sheet(sheet).Origin({16, 32});
     * Spawn<collider::Rect>().Fit(hero, 4); // o 4 px mniejszy z każdej strony niż klatka
     * @endcode
     * @param sprite Sprite-rodzeństwo.
     * @param inset Ile pikseli odjąć z każdej strony (ujemne - powiększenie).
     * @return Ten sam collider.
     */
    template <typename Self>
        requires std::same_as<S, shapes::Rect>
    Self& Fit(this Self& self, const visual::Sprite& sprite, float inset = 0.0f) {
        const Vector2 frame = sprite.FrameSize();
        const Vector2 origin = sprite.Pivot();
        const Vector2 position = sprite.Position();
        self.Position({position.x - origin.x + inset, position.y - origin.y + inset});
        self.Size({std::max(0.0f, frame.x - 2.0f * inset), std::max(0.0f, frame.y - 2.0f * inset)});
        return self;
    }

    /** @copydoc CollisionShape::WorldParts */
    void WorldParts(std::vector<ConvexPart>& parts) const override {
        const std::size_t first = parts.size();
        this->ConvexParts(parts);
        const AffineMatrix2D world = this->WorldTransform();
        for (std::size_t i = first; i < parts.size(); ++i)
            parts[i] = parts[i].Transformed(world);
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
