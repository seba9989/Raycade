/**
 * @file Scene.hpp
 * @brief Klasa Scene - zbiór obiektów i logiki jednego ekranu.
 */
#pragma once

#include <algorithm>

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <raycade/entity/Contextable.hpp>
#include <raycade/entity/Entity.hpp>
#include <raycade/input/Input.hpp>
#include <raycade/physics/PhysicsWorld.hpp>
#include <raycade/render/PrimitiveRenderer.hpp>
#include <raycade/scene/PointerSystem.hpp>
#include <raycade/scene/SceneCamera.hpp>

/**
 * @brief Zbiór obiektów i funkcji logicznych jednego ekranu silnika.
 * @details Silnik aktualizuje i rysuje wyłącznie aktywną scenę. Obiekty dodane
 * przez Spawn() są korzeniami drzew i mogą tworzyć własne dzieci.
 *
 * Kontekst sceny (ProvideContext<T>(), SetContext(), Context<T>() z Contextable) widzą
 * wszystkie obiekty sceny. Wartości należą do sceny: znikają przy UnloadScene() i powstają od nowa
 * przy ponownym wczytaniu - patrz @ref entity_kontekst.
 */
class Scene : public Contextable {
  public:
    /**
     * @brief Dodaje obiekt do sceny.
     * @tparam T Kształt z przestrzeni nazw @ref shapes albo własna klasa dziedzicząca
     * po Entity, np. `Spawn<shapes::Rect>()`.
     * @return Referencja do nowego obiektu, ważna przez cały czas życia sceny.
     */
    template <typename T> T& Spawn() {
        auto entity = std::make_unique<T>();
        T& ref = *entity;
        entities_.push_back(std::move(entity));
        ref.Attach(links_); // OnMount() obiektu i dzieci z jego konstruktora
        return ref;
    }

    /** @brief Pusta scena. */
    Scene() = default;
    /** @brief Obiekty pamiętają adres sceny, więc sceny nie można kopiować. */
    Scene(const Scene&) = delete;
    /** @brief Obiekty pamiętają adres sceny, więc sceny nie można przypisywać. */
    Scene& operator=(const Scene&) = delete;

    /**
     * @brief Ustawia logikę sceny wykonywaną co klatkę.
     * @param update Funkcja otrzymująca czas dt w sekundach.
     * @return Ta sama scena.
     */
    Scene& OnUpdate(std::function<void(float dt)> update) {
        update_ = std::move(update);
        return *this;
    }

    /**
     * @brief Ustawia logikę sceny wykonywaną ze stałym krokiem czasu (fizyka, kolizje).
     * @details Wywoływana zero, raz albo kilka razy na klatkę - tak, żeby średnio
     * wypadało 1 / Engine::FixedStep() wywołań na sekundę niezależnie od liczby klatek.
     * Wykonuje się przed OnUpdate() sceny. W kroku stałym odczytuje się stan trzymany
     * (KeyDown(), Axis()); jednorazowe naciśnięcia (KeyPressed()) należą do OnUpdate(), bo
     * w klatce bez kroku stałego zostałyby zgubione.
     * @param update Funkcja otrzymująca stały krok w sekundach.
     * @return Ta sama scena.
     */
    Scene& OnFixedUpdate(std::function<void(float step)> update) {
        fixedUpdate_ = std::move(update);
        return *this;
    }

    /**
     * @brief Ustawia dodatkowe rysowanie wykonywane po obiektach sceny.
     * @details Rysuje w układzie okna (bez kamery) - miejsce na podpisy i HUD.
     * @param draw Funkcja otrzymująca renderer.
     * @return Ta sama scena.
     */
    Scene& OnDraw(std::function<void(PrimitiveRenderer&)> draw) {
        draw_ = std::move(draw);
        return *this;
    }

    /**
     * @brief Ustawia rysowanie w układzie świata (przez kamerę), przed obiektami świata.
     * @details Miejsce na tło, siatkę albo granice poziomu, które mają się przesuwać i
     * przybliżać razem ze światem.
     * @param draw Funkcja otrzymująca renderer.
     * @return Ta sama scena.
     */
    Scene& OnDrawWorld(std::function<void(PrimitiveRenderer&)> draw) {
        drawWorld_ = std::move(draw);
        return *this;
    }

    /**
     * @brief Udostępnia kamerę sceny.
     * @details Domyślna kamera niczego nie zmienia. Patrz SceneCamera.
     * @return Kamera tej sceny.
     */
    SceneCamera& Camera() { return camera_; }

    /** @brief Udostępnia kamerę sceny tylko do odczytu. */
    const SceneCamera& Camera() const { return camera_; }

    /**
     * @name Fokus klawiatury
     * Dotyczy obiektów z cechą Focusable - patrz jej opis.
     */
    /** @{ */

    /** @brief Zwraca obiekt z fokusem albo nullptr. */
    const Entity* Focused() const { return focus_.focused ? focus_.focused->Owner() : nullptr; }

    /**
     * @brief Zabiera fokus wszystkim obiektom sceny.
     * @return Ta sama scena.
     */
    Scene& ClearFocus() {
        focus_.focused = nullptr;
        return *this;
    }

    /** @brief Czy obiekt z fokusem przejął klawiaturę (np. pole tekstowe). */
    bool IsKeyboardCaptured() const { return focus_.focused && focus_.focused->CapturesKeyboard(); }

    /**
     * @brief Włącza lub wyłącza przechodzenie fokusu klawiszem Tab (domyślnie włączone).
     * @param value Czy Tab i Shift+Tab zmieniają fokus.
     * @return Ta sama scena.
     */
    Scene& TabNavigation(bool value = true) {
        tabNavigation_ = value;
        return *this;
    }

    /** @brief Sprawdza, czy Tab przenosi fokus (TabNavigation()). */
    bool IsTabNavigation() const { return tabNavigation_; }

    /** @} */

    /** @name Fizyka */
    /** @{ */

    /**
     * @brief Ustawia grawitację sceny w pikselach/s² (domyślnie {0, 980}).
     * @details Czyta ją CollisionObject::Gravity(); postać dodaje ją sama do swojej prędkości.
     * @param gravity Wektor grawitacji (oś y w dół).
     * @return Ta sama scena.
     */
    Scene& Gravity(Vector2 gravity) {
        physics_.Gravity(gravity);
        return *this;
    }

    /** @brief Podaje grawitację sceny. */
    Vector2 Gravity() const { return physics_.Gravity(); }

    /**
     * @brief Rysuje obrysy wszystkich colliderów (jak „Visible Collision Shapes” w Godocie).
     * @details StaticBody - niebieskie, CharacterBody - zielone, Area - pomarańczowe,
     * wyłączone collidery - szare.
     * @param value Czy rysować.
     * @return Ta sama scena.
     */
    Scene& DebugColliders(bool value = true) {
        physics_.Debug(value);
        return *this;
    }

    /** @brief Sprawdza, czy obrysy colliderów są rysowane. */
    bool IsDebugColliders() const { return physics_.IsDebug(); }

    /**
     * @brief Szuka pierwszego ciała na odcinku from-to (RayCast2D z Godota).
     * @details Trafia ciała (StaticBody, CharacterBody), nie obszary. Promień zaczynający się
     * wewnątrz collidera go nie trafia; platformę jednostronną trafia tylko od jej „góry”.
     * Współrzędne są w układzie sceny (świata).
     * @code{.cpp}
     * // czy przeciwnik widzi gracza - nic z warstwy World nie stoi między nimi
     * const bool sees = !scene.Raycast(enemy.WorldPosition(), player.WorldPosition(), World);
     * @endcode
     * @param from Początek promienia.
     * @param to Koniec promienia.
     * @param mask Warstwy, które promień trafia (domyślnie wszystkie).
     * @param exclude Obiekt pomijany (np. ten, który strzela).
     * @return Najbliższe trafienie albo std::nullopt.
     */
    std::optional<RayHit> Raycast(
        Vector2 from,
        Vector2 to,
        std::uint32_t mask = kAllLayers,
        const CollisionObject* exclude = nullptr
    ) const {
        return physics_.Raycast(from, to, mask, exclude);
    }

    /**
     * @brief Ciała nachodzące na prostokąt (np. „czy jest tu miejsce”, zaznaczenie obszarem).
     * @param area Prostokąt w układzie sceny.
     * @param mask Warstwy, które są sprawdzane (domyślnie wszystkie).
     * @param exclude Obiekt pomijany.
     */
    std::vector<PhysicsBody*> Overlap(
        Rectangle area,
        std::uint32_t mask = kAllLayers,
        const CollisionObject* exclude = nullptr
    ) const {
        const ConvexPart part = ConvexPart::Polygon(
            {{area.x, area.y},
             {area.x + area.width, area.y},
             {area.x + area.width, area.y + area.height},
             {area.x, area.y + area.height}}
        );
        return physics_.Overlap(part, mask, exclude);
    }

    /**
     * @brief Ciała nachodzące na koło (np. zasięg wybuchu).
     * @param center Środek w układzie sceny.
     * @param radius Promień.
     * @param mask Warstwy, które są sprawdzane (domyślnie wszystkie).
     * @param exclude Obiekt pomijany.
     */
    std::vector<PhysicsBody*> Overlap(
        Vector2 center,
        float radius,
        std::uint32_t mask = kAllLayers,
        const CollisionObject* exclude = nullptr
    ) const {
        return physics_.Overlap(ConvexPart::Ellipse(center, {radius, radius}), mask, exclude);
    }

    /** @} */

    /**
     * @brief Wykonuje jeden krok stały sceny: OnFixedUpdate(), krok obiektów i zdarzenia fizyki.
     * @details Wywoływana przez silnik w pętli kroku stałego.
     * @param step Stały krok w sekundach.
     * @param input Stan wejścia.
     */
    void FixedUpdate(float step, const Input& input);

    /**
     * @brief Aktualizuje logikę sceny i jej obiekty.
     * @param dt Czas od poprzedniej klatki w sekundach.
     * @param input Stan wejścia.
     */
    void Update(float dt, const Input& input);

    /**
     * @brief Rysuje scenę: świat przez kamerę, potem obiekty ekranu i OnDraw().
     * @details Kolejność: OnDrawWorld() i obiekty świata (przez kamerę), następnie obiekty
     * ekranu (ScreenSpace()) i OnDraw() w układzie okna. W każdej grupie - od najniższej
     * warstwy, a w tej samej warstwie - według dodania.
     * @param renderer Renderer prymitywów.
     */
    void Draw(PrimitiveRenderer& renderer) const;

  protected:
    /** @brief Poziom wyżej w łańcuchu kontekstu sceny to silnik. */
    const Contextable* ContextParent() const override { return ContextRoot(); }

  private:
    /** @brief Wywołuje OnBlur() / OnFocus(), jeśli fokus zmienił się od ostatniego razu. */
    void NotifyFocus();
    /**
     * @brief Usuwa obiekty oznaczone przez Entity::Destroy() (rekurencyjnie, z dziećmi).
     * @param entities Lista obiektów jednego poziomu drzewa.
     */
    void RemoveDestroyed(std::vector<std::unique_ptr<Entity>>& entities);

    /**
     * @brief Przenosi fokus na następny (+1) albo poprzedni (-1) obiekt w kolejności Taba.
     * @param step Kierunek.
     */
    void MoveFocus(int step);

    /**
     * @brief Rysuje obiekty najwyższego poziomu jednego rodzaju według warstw.
     * @param renderer Renderer prymitywów.
     * @param screenSpace true - obiekty ekranu, false - obiekty świata.
     */
    void DrawRoots(PrimitiveRenderer& renderer, bool screenSpace) const;

    /** Kamera sceny; obiekty pamiętają jej adres (dlatego sceny nie można kopiować). */
    SceneCamera camera_;
    FocusState
        focus_; /**< Fokus klawiatury (niszczony po obiektach, które się z niego wypisują). */
    /** Powiązania ze sceną wspólne dla wszystkich jej obiektów (patrz SceneLinks). */
    /** Świat fizyki; zna listę obiektów sceny (entities_), ale używa jej dopiero w klatce. */
    PhysicsWorld physics_{entities_};
    SceneLinks links_{this, this, &camera_, &focus_, &physics_};
    PointerSystem pointer_;     /**< Wybór obiektów pod kursorem i odbiorców zdarzeń myszy. */
    bool tabNavigation_ = true; /**< Czy Tab przenosi fokus. */
    /** Obiekty najwyższego poziomu. Pole jest niszczone przed klasą bazową Contextable,
     *  więc obiekty znikają przed kontekstem sceny. */
    std::vector<std::unique_ptr<Entity>> entities_;
    std::function<void(float)> update_;            /**< Logika sceny co klatkę. */
    std::function<void(float)> fixedUpdate_;       /**< Logika sceny ze stałym krokiem. */
    std::function<void(PrimitiveRenderer&)> draw_; /**< Rysowanie w układzie okna, po obiektach. */
    std::function<void(PrimitiveRenderer&)> drawWorld_; /**< Rysowanie w układzie świata. */
};
