/**
 * @file CharacterBody.hpp
 * @brief Klasa CharacterBody - postać sterowana kodem z MoveAndSlide().
 */
#pragma once

#include <cmath>
#include <vector>

#include <raycade/physics/PhysicsBody.hpp>

/**
 * @brief Ciało sterowane kodem: ruch przez Velocity() i MoveAndSlide() (CharacterBody2D z Godota).
 * @details Postać sama ustawia prędkość (sterowanie, skok, grawitacja), a MoveAndSlide()
 * przesuwa ją o `Velocity() * krok`, wypycha z innych ciał i ślizga się wzdłuż nich: składowa
 * prędkości w głąb ściany albo podłogi jest zerowana. Po ruchu IsOnFloor(), IsOnWall()
 * i IsOnCeiling() mówią, czego postać dotyka.
 *
 * MoveAndSlide() wywołuje się w kroku stałym - w FixedTick() albo OnFixedUpdate().
 * Na podłodze (pochylonej mniej niż FloorMaxAngle()) postać jest wypychana pionowo w górę,
 * więc nie zsuwa się z pochylni i wchodzi na nią przy ruchu w bok.
 * @code{.cpp}
 * class Player : public CharacterBody {
 *   protected:
 *     void Tick(float, const Input& input) override {
 *         if (input.KeyPressed(KEY_SPACE)) jump_ = true; // jednorazowe naciśnięcie - co klatkę
 *     }
 *     void FixedTick(float step, const Input& input) override {
 *         Vector2 v = Velocity();
 *         v.x = (input.KeyDown(KEY_D) - input.KeyDown(KEY_A)) * 250.0f;
 *         v.y += Gravity().y * step;
 *         if (jump_ && IsOnFloor()) v.y = -500.0f;
 *         jump_ = false;
 *         Velocity(v).MoveAndSlide();
 *     }
 *   private:
 *     bool jump_ = false;
 * };
 * @endcode
 */
class CharacterBody : public PhysicsBody {
  public:
    /**
     * @brief Ustawia prędkość w pikselach na sekundę (układ sceny).
     * @param velocity Prędkość.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Velocity(this Self& self, Vector2 velocity) {
        self.velocity_ = velocity;
        return self;
    }

    /** @brief Podaje prędkość (po MoveAndSlide() - z wyzerowaną składową w głąb przeszkód). */
    Vector2 Velocity() const { return velocity_; }

    /**
     * @brief Ustawia kierunek „w górę” - od niego zależy, co jest podłogą, a co sufitem.
     * @param up Wektor jednostkowy, domyślnie {0, -1} (oś y w dół).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& UpDirection(this Self& self, Vector2 up) {
        self.up_ = up;
        return self;
    }

    /** @brief Podaje kierunek „w górę”. */
    Vector2 UpDirection() const { return up_; }

    /**
     * @brief Ustawia największe pochylenie podłogi - bardziej stroma powierzchnia jest ścianą.
     * @param radians Kąt w radianach, domyślnie PI / 4 (45°).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& FloorMaxAngle(this Self& self, float radians) {
        self.floorMaxAngle_ = radians;
        return self;
    }

    /** @brief Podaje największe pochylenie podłogi w radianach. */
    float FloorMaxAngle() const { return floorMaxAngle_; }

    /**
     * @brief Ustawia, z jakiej odległości postać jest przyciągana do podłogi pod nią.
     * @details Schodząc z pochylni, postać przesuwa się w bok szybciej, niż grawitacja ściąga ją
     * w dół, więc bez przyciągania odrywałaby się od powierzchni (IsOnFloor() == false - nie da
     * się skoczyć). MoveAndSlide() przyciąga postać, która przed ruchem stała na podłodze i nie
     * leci w górę. Potrzebna odległość to mniej więcej prędkość * krok * tg(kąt pochylni).
     * @param length Odległość w pikselach; 0 wyłącza przyciąganie. Domyślnie 8.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& FloorSnapLength(this Self& self, float length) {
        self.floorSnapLength_ = length;
        return self;
    }

    /** @brief Podaje odległość przyciągania do podłogi w pikselach. */
    float FloorSnapLength() const { return floorSnapLength_; }

    /**
     * @brief Przesuwa postać o Velocity() * krok, wypychając ją z innych ciał i ślizgając wzdłuż.
     * @details Krokiem jest bieżący krok stały (w FixedTick()) albo czas klatki (w Tick()).
     * Długi ruch jest dzielony na krótsze odcinki, żeby szybka postać nie przeleciała przez
     * cienką ścianę. Postać zderza się z ciałami (StaticBody, CharacterBody), których warstwy
     * pasują do jej CollisionMask(); obszary Area jej nie zatrzymują.
     * @return true, gdy postać w trakcie ruchu na coś wpadła.
     */
    bool MoveAndSlide();

    /** @brief Sprawdza, czy po ostatnim MoveAndSlide() postać stoi na podłodze. */
    bool IsOnFloor() const { return onFloor_; }

    /** @brief Sprawdza, czy po ostatnim MoveAndSlide() postać dotyka ściany. */
    bool IsOnWall() const { return onWall_; }

    /** @brief Sprawdza, czy po ostatnim MoveAndSlide() postać dotyka sufitu. */
    bool IsOnCeiling() const { return onCeiling_; }

    /**
     * @brief Prędkość podłogi, na której stała postać (np. jadącej platformy), w px/s.
     * @details MoveAndSlide() przesuwa postać razem z podłogą, na której stoi: platforma
     * przesuwana w kodzie (Position(), Translate(), Rotate()) wiezie postać, także po łuku, gdy
     * się obraca. Gdy postać opuszcza podłogę (skok, zejście z krawędzi), prędkość platformy jest
     * dodawana do Velocity() - skok z jadącej platformy nie urywa się w miejscu.
     * @return Przesunięcie podłogi pod postacią w ostatnim kroku podzielone przez krok; {0, 0}
     * dla nieruchomej podłogi.
     */
    Vector2 PlatformVelocity() const { return platformVelocity_; }

    /** @brief Normalna podłogi z ostatniego MoveAndSlide() (gdy IsOnFloor()). */
    Vector2 FloorNormal() const { return floorNormal_; }

    /** @brief Zderzenia z ostatniego MoveAndSlide() (patrz Collision o czasie ważności). */
    const std::vector<Collision>& SlideCollisions() const { return collisions_; }

  private:
    friend class PhysicsWorld;

    Vector2 velocity_{};                /**< Prędkość w pikselach na sekundę. */
    Vector2 up_{0.0f, -1.0f};           /**< Kierunek „w górę”. */
    float floorMaxAngle_ = PI / 4.0f;   /**< Największe pochylenie podłogi. */
    float floorSnapLength_ = 8.0f;      /**< Przyciąganie do podłogi (px). */
    bool onFloor_ = false;              /**< Stoi na podłodze. */
    bool onWall_ = false;               /**< Dotyka ściany. */
    bool onCeiling_ = false;            /**< Dotyka sufitu. */
    Vector2 floorNormal_{0.0f, -1.0f};  /**< Normalna podłogi. */
    std::vector<Collision> collisions_; /**< Zderzenia z ostatniego ruchu. */
    // Podłoga, na której postać stała po ostatnim ruchu - do wożenia przez platformy. Wskaźnik
    // jest tylko porównywany z listą ciał świata, zanim zostanie użyty (podłoga mogła zniknąć).
    const PhysicsBody* floorBody_ = nullptr; /**< Ciało pod postacią albo nullptr. */
    Vector2 floorLocal_{};                   /**< Punkt pod postacią w układzie podłogi. */
    Vector2 floorWorld_{};                   /**< Ten punkt w układzie sceny przy zapisie. */
    Vector2 platformVelocity_{};             /**< Prędkość podłogi pod postacią. */
};
