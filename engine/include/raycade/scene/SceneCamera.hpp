/**
 * @file SceneCamera.hpp
 * @brief Kamera 2D sceny: przesuwanie, przybliżanie i obrót widoku świata.
 */
#pragma once

#include <raylib.h>

#include <raycade/entity/Entity.hpp>
#include <raycade/math/AffineMatrix2D.hpp>

#include <cmath>
#include <functional>
#include <type_traits>

/**
 * @brief Kamera 2D sceny (nakładka na Camera2D z raylib).
 * @details Kamera pokazuje punkt świata Target() w miejscu ekranu Offset(), z przybliżeniem
 * Zoom() i obrotem Rotation(). Domyślna kamera niczego nie zmienia: punkt świata (x, y) jest
 * w pikselu (x, y) okna.
 *
 * Kamera dotyczy obiektów świata. Obiekty oznaczone Entity::ScreenSpace() (np. HUD, menu)
 * są rysowane w układzie okna, nad światem. Trafienia myszą scena przelicza przez kamerę
 * sama, więc OnClick, OnDrag i IsHovered() działają także po przesunięciu i przybliżeniu.
 * @code{.cpp}
 * auto& player = scene.Spawn<game::Player>().Position({0, 0});
 * scene.Camera()
 *     .Offset({500, 350})   // środek okna 1000x700
 *     .Zoom(2.0f)
 *     .Follow(player, 5.0f); // płynne podążanie
 * @endcode
 *
 * Nazwa SceneCamera, bo nazwę Camera zajmuje raylib (typedef Camera3D Camera).
 */
class SceneCamera {
  public:
    /**
     * @brief Ustawia punkt świata pokazywany w miejscu Offset().
     * @param target Punkt w układzie świata.
     * @return Ta sama kamera.
     */
    SceneCamera& Target(Vector2 target) {
        camera_.target = target;
        return *this;
    }

    /** @brief Zwraca punkt świata pokazywany w miejscu Offset(). */
    Vector2 Target() const { return camera_.target; }

    /**
     * @brief Ustawia miejsce na ekranie, w którym widać Target() (zwykle środek okna).
     * @param offset Punkt w układzie okna.
     * @return Ta sama kamera.
     */
    SceneCamera& Offset(Vector2 offset) {
        camera_.offset = offset;
        return *this;
    }

    /** @brief Zwraca miejsce na ekranie, w którym widać Target(). */
    Vector2 Offset() const { return camera_.offset; }

    /**
     * @brief Ustawia przybliżenie (1 - bez zmian, 2 - dwa razy większe).
     * @param zoom Przybliżenie większe od 0.
     * @return Ta sama kamera.
     */
    SceneCamera& Zoom(float zoom) {
        camera_.zoom = zoom;
        return *this;
    }

    /** @brief Zwraca przybliżenie. */
    float Zoom() const { return camera_.zoom; }

    /**
     * @brief Ustawia obrót widoku wokół Offset().
     * @param radians Kąt w radianach (jak Rotate() obiektów).
     * @return Ta sama kamera.
     */
    SceneCamera& Rotation(float radians) {
        camera_.rotation = radians * RAD2DEG; // raylib przyjmuje stopnie
        return *this;
    }

    /** @brief Zwraca obrót widoku w radianach. */
    float Rotation() const { return camera_.rotation * DEG2RAD; }

    /**
     * @brief Ustawia podążanie za obiektem: co klatkę Target() zbliża się do jego pozycji.
     * @details Gdy śledzony obiekt sceny (albo jego przodek) zostanie usunięty przez
     * Entity::Destroy(), scena sama wyłącza podążanie. Obiekt spoza sceny musi żyć co najmniej
     * tak długo, jak kamera za nim podąża.
     * @tparam T Typ obiektu z metodą WorldPosition() (każdy Entity).
     * @param target Obiekt, za którym podąża kamera.
     * @param speed 0 - natychmiast; większa wartość - szybsze doganianie (np. 5 = płynnie).
     * @return Ta sama kamera.
     */
    template <typename T> SceneCamera& Follow(const T& target, float speed = 0.0f) {
        follow_ = [&target] { return target.WorldPosition(); };
        if constexpr (std::is_base_of_v<Entity, T>)
            followed_ = &target;
        else
            followed_ = nullptr;
        followSpeed_ = speed;
        camera_.target = follow_(); // bez "dojazdu" z punktu (0, 0) w pierwszej klatce
        return *this;
    }

    /**
     * @brief Wyłącza podążanie za obiektem; Target() zostaje tam, gdzie był.
     * @return Ta sama kamera.
     */
    SceneCamera& StopFollowing() {
        follow_ = nullptr;
        return *this;
    }

    /** @brief Sprawdza, czy kamera podąża za obiektem. */
    bool IsFollowing() const { return static_cast<bool>(follow_); }

    /** @brief Podaje śledzony obiekt sceny albo nullptr (brak podążania albo cel spoza Entity). */
    const Entity* Followed() const { return follow_ ? followed_ : nullptr; }

    /**
     * @brief Podaje przekształcenie świat -> okno jako macierz silnika.
     * @details Złożenie z macierzą obiektu daje drogę punktu lokalnego aż do okna, np.
     * `camera.Matrix() * transform`. Odwrotność (AffineMatrix2D::InverseApply()) przelicza
     * okno na świat jak ScreenToWorld().
     * @return Macierz z GetCameraMatrix2D() (offset, obrót, zoom, cel).
     */
    AffineMatrix2D Matrix() const {
        const ::Matrix m = ::GetCameraMatrix2D(camera_);
        return {m.m0, m.m1, m.m4, m.m5, m.m12, m.m13};
    }

    /**
     * @brief Przelicza punkt okna (np. kursor) na punkt świata.
     * @param screen Punkt w układzie okna.
     * @return Punkt w układzie świata.
     */
    Vector2 ScreenToWorld(Vector2 screen) const { return ::GetScreenToWorld2D(screen, camera_); }

    /**
     * @brief Przelicza punkt świata na punkt okna.
     * @param world Punkt w układzie świata.
     * @return Punkt w układzie okna.
     */
    Vector2 WorldToScreen(Vector2 world) const { return ::GetWorldToScreen2D(world, camera_); }

    /** @brief Udostępnia kamerę raylib (np. do własnego BeginMode2D). */
    const Camera2D& Raw() const { return camera_; }

  private:
    friend class Scene; // wywołuje Update() co klatkę

    /**
     * @brief Przesuwa Target() za śledzonym obiektem. Wywoływana przez scenę co klatkę.
     * @param dt Czas od poprzedniej klatki w sekundach.
     */
    void Update(float dt);

    Camera2D camera_{{0, 0}, {0, 0}, 0.0f, 1.0f}; /**< Offset, target, obrót (stopnie), zoom. */
    std::function<Vector2()> follow_;             /**< Pozycja śledzonego obiektu. */
    const Entity* followed_ = nullptr;            /**< Śledzony obiekt (do Destroy()). */
    float followSpeed_ = 0.0f;                    /**< Szybkość doganiania (0 = natychmiast). */
};
