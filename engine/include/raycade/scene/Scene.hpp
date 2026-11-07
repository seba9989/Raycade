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
#include <raycade/render/PrimitiveRenderer.hpp>
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

    /**
     * @brief Aktualizuje logikę sceny i jej obiekty.
     * @param dt Czas od poprzedniej klatki w sekundach.
     * @param input Stan wejścia.
     */
    void Update(float dt, const Input& input) {
        if (update_) update_(dt);
        // pętla po indeksie: zdarzenie obiektu może wywołać Spawn() i powiększyć wektor
        // Tab działa także przy przejętej klawiaturze - inaczej z pola tekstowego nie dałoby
        // się przejść dalej
        if (tabNavigation_ && input.SystemKeyPressed(KEY_TAB))
            MoveFocus(input.SystemKeyDown(KEY_LEFT_SHIFT) || input.SystemKeyDown(KEY_RIGHT_SHIFT)
                          ? -1
                          : 1);

        // trafienia myszą liczone raz, przed zmianami w tej klatce (patrz Entity::Pointer)
        const PointerFrame pointer = Entity::Pointer(entities_, input, camera_);
        // kliknięcie daje fokus trafionemu obiektowi z cechą Focusable, a w cokolwiek innego -
        // zabiera
        if (pointer.pressed) focus_.focused = pointer.focus;
        NotifyFocus(); // także zmiany z Focus()/Blur() wywołanych w poprzedniej klatce

        PointerFrame frame = pointer;
        frame.focused = focus_.focused ? focus_.focused->Owner() : nullptr;
        for (std::size_t i = 0; i < entities_.size(); ++i)
            entities_[i]->Update(dt, input, frame);
        // po ruchu obiektów, żeby kamera w tej samej klatce pokazała ich nowe położenie
        camera_.Update(dt);
    }

    /**
     * @brief Rysuje scenę: świat przez kamerę, potem obiekty ekranu i OnDraw().
     * @details Kolejność: OnDrawWorld() i obiekty świata (przez kamerę), następnie obiekty
     * ekranu (ScreenSpace()) i OnDraw() w układzie okna. W każdej grupie - od najniższej
     * warstwy, a w tej samej warstwie - według dodania.
     * @param renderer Renderer prymitywów.
     */
    void Draw(PrimitiveRenderer& renderer) const {
        ::BeginMode2D(camera_.Raw());
        if (drawWorld_) drawWorld_(renderer.Reset().TargetScreen());
        DrawRoots(renderer, false);
        ::EndMode2D();
        DrawRoots(renderer, true);
        if (draw_) draw_(renderer.Reset().TargetScreen());
    }

  protected:
    /** @brief Poziom wyżej w łańcuchu kontekstu sceny to silnik. */
    const Contextable* ContextParent() const override { return ContextRoot(); }

  private:
    /** @brief Wywołuje OnBlur() / OnFocus(), jeśli fokus zmienił się od ostatniego razu. */
    void NotifyFocus() {
        if (focus_.focused == focus_.notified) return;
        if (focus_.notified && focus_.notified->blur_) focus_.notified->blur_();
        focus_.notified = focus_.focused;
        if (focus_.notified && focus_.notified->focus_) focus_.notified->focus_();
    }

    /**
     * @brief Przenosi fokus na następny (+1) albo poprzedni (-1) obiekt w kolejności Taba.
     * @param step Kierunek.
     */
    void MoveFocus(int step) {
        const std::vector<const Focusable*> order = Focusable::TabOrder(focus_);
        if (order.empty()) return;
        const auto current = std::find(order.begin(), order.end(), focus_.focused);
        const int size = static_cast<int>(order.size());
        int index = current == order.end() ? (step > 0 ? -1 : size)
                                           : static_cast<int>(current - order.begin());
        index = ((index + step) % size + size) % size;
        focus_.focused = order[static_cast<std::size_t>(index)];
    }

    /**
     * @brief Rysuje obiekty najwyższego poziomu jednego rodzaju według warstw.
     * @param renderer Renderer prymitywów.
     * @param screenSpace true - obiekty ekranu, false - obiekty świata.
     */
    void DrawRoots(PrimitiveRenderer& renderer, bool screenSpace) const {
        std::vector<const Drawable*> order;
        order.reserve(entities_.size());
        for (const auto& entity : entities_)
            if (entity->IsScreenSpace() == screenSpace) order.push_back(entity.get());
        for (const Drawable* entity : Drawable::ByLayer(std::move(order)))
            entity->Render(renderer);
    }

    /** Kamera sceny; obiekty pamiętają jej adres (dlatego sceny nie można kopiować). */
    SceneCamera camera_;
    FocusState
        focus_; /**< Fokus klawiatury (niszczony po obiektach, które się z niego wypisują). */
    /** Powiązania ze sceną wspólne dla wszystkich jej obiektów (patrz SceneLinks). */
    SceneLinks links_{this, this, &camera_, &focus_};
    bool tabNavigation_ = true; /**< Czy Tab przenosi fokus. */
    /** Obiekty najwyższego poziomu. Pole jest niszczone przed klasą bazową Contextable,
     *  więc obiekty znikają przed kontekstem sceny. */
    std::vector<std::unique_ptr<Entity>> entities_;
    std::function<void(float)> update_;            /**< Logika sceny co klatkę. */
    std::function<void(PrimitiveRenderer&)> draw_; /**< Rysowanie w układzie okna, po obiektach. */
    std::function<void(PrimitiveRenderer&)> drawWorld_; /**< Rysowanie w układzie świata. */
};
