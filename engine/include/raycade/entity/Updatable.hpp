/**
 * @file Updatable.hpp
 * @brief Aktualizacja jednego obiektu i obsługa jego zdarzeń wejścia.
 */
#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include <raycade/input/Input.hpp>

/**
 * @brief Typ funkcji zdarzenia otrzymującej obiekt jako Self&.
 * @details std::type_identity_t wyłącza dedukcję Self z tego parametru: Self wynika tylko
 * z obiektu, na którym wywołano metodę, a lambda zostaje skonwertowana na std::function.
 * @tparam Self Konkretny typ obiektu.
 * @tparam Args Dodatkowe argumenty (np. czas dt).
 */
template <typename Self, typename... Args>
using Callback = std::type_identity_t<std::function<void(Self&, Args...)>>;

/**
 * @brief Zdarzenia myszy przydzielone jednemu obiektowi w bieżącej klatce.
 * @details Wypełnia je Entity::Update() z wyniku PointerSystem (PointerFrame), więc obiekt nie
 * sprawdza kursora sam - patrz @ref entity_trafienia.
 */
struct PointerEvents {
    /** Kursor nad obiektem (albo nad jego potomkiem) i nic go nie zasłania. */
    bool hovered = false;
    bool pressed = false;   /**< Obiekt dostaje OnClick w tej klatce. */
    bool released = false;  /**< Obiekt dostaje OnMouseReleased w tej klatce. */
    bool scrolled = false;  /**< Obiekt dostaje OnScroll w tej klatce. */
    bool dragStart = false; /**< Obiekt zaczyna przeciąganie w tej klatce. */
};

/**
 * @brief Obiekt aktualizowany co klatkę: Tick(), OnUpdate() i zdarzenia wejścia.
 * @details Klasa tylko przechowuje i wywołuje własne zdarzenia. Nie zna geometrii ani drzewa:
 * - obszar trafienia (Contains()) i aktualizacja dzieci należą do Entity,
 * - to, który obiekt jest pod kursorem i kto dostaje zdarzenie myszy, ustala PointerSystem
 *   sceny.
 */
class Updatable {
  public:
    virtual ~Updatable() = default;

    /**
     * @brief Sprawdza, czy kursor jest nad obiektem.
     * @details Tylko najwyżej narysowany obiekt pod kursorem i jego przodkowie są „najechani”
     * - obiekt zasłonięty przez inny nie jest.
     */
    bool IsHovered() const { return hovered_; }

    /** @brief Sprawdza, czy obiekt jest właśnie przeciągany (patrz OnDrag()). */
    bool IsDragging() const { return dragging_; }

    /**
     * @name Zdarzenia
     * Callback otrzymuje obiekt jako jego konkretny typ (np. `shapes::Circle&`), bo metody
     * używają jawnego parametru obiektu z C++23 (`this Self& self`). Obiekty sceny nie są
     * przenoszone w pamięci, więc callback może bezpiecznie trzymać referencję do obiektu.
     */
    /** @{ */

    /**
     * @brief Ustawia funkcję aktualizacji wywoływaną co klatkę.
     * @param callback Funkcja otrzymująca obiekt i czas dt w sekundach.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnUpdate(this Self& self, Callback<Self, float> callback) {
        self.update_ = [&self, fn = std::move(callback)](float dt) { fn(self, dt); };
        return self;
    }

    /**
     * @brief Ustawia funkcję wywoływaną w każdym kroku stałym (fizyka, ruch postaci).
     * @details Działa jak Scene::OnFixedUpdate(), ale dla jednego obiektu: zero, raz albo kilka
     * razy na klatkę, ze stałym krokiem Engine::FixedStep(). Tu wywołuje się
     * CharacterBody::MoveAndSlide(). Jednorazowe naciśnięcia klawiszy (KeyPressed()) należą do
     * OnUpdate(), bo w klatce bez kroku stałego zostałyby zgubione.
     * @param callback Funkcja otrzymująca obiekt i krok w sekundach.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnFixedUpdate(this Self& self, Callback<Self, float> callback) {
        self.fixedUpdate_ = [&self, fn = std::move(callback)](float step) { fn(self, step); };
        return self;
    }

    /**
     * @brief Ustawia reakcję na kliknięcie lewym przyciskiem wewnątrz obiektu.
     * @param callback Funkcja otrzymująca obiekt.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnClick(this Self& self, Callback<Self> callback) {
        self.click_ = Bind(self, std::move(callback));
        return self;
    }

    /**
     * @brief Ustawia reakcję na wejście kursora w obszar obiektu.
     * @param callback Funkcja otrzymująca obiekt.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnMouseEnter(this Self& self, Callback<Self> callback) {
        self.mouseEnter_ = Bind(self, std::move(callback));
        return self;
    }

    /**
     * @brief Ustawia reakcję na opuszczenie obiektu przez kursor.
     * @param callback Funkcja otrzymująca obiekt.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnMouseLeave(this Self& self, Callback<Self> callback) {
        self.mouseLeave_ = Bind(self, std::move(callback));
        return self;
    }

    /**
     * @brief Ustawia reakcję na naciśnięcie wybranego klawisza.
     * @details Można podpiąć kilka różnych klawiszy.
     * @param key Kod klawisza raylib.
     * @param callback Funkcja otrzymująca obiekt.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnKeyPressed(this Self& self, int key, Callback<Self> callback) {
        self.keys_.emplace_back(key, Bind(self, std::move(callback)));
        return self;
    }

    /**
     * @brief Ustawia reakcję na puszczenie wybranego klawisza.
     * @details Można podpiąć kilka różnych klawiszy.
     * @param key Kod klawisza raylib.
     * @param callback Funkcja otrzymująca obiekt.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& OnKeyReleased(this Self& self, int key, Callback<Self> callback) {
        self.keysReleased_.emplace_back(key, Bind(self, std::move(callback)));
        return self;
    }

    /**
     * @brief Ustawia reakcję na puszczenie lewego przycisku myszy nad obiektem.
     * @param callback Funkcja otrzymująca obiekt.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnMouseReleased(this Self& self, Callback<Self> callback) {
        self.mouseReleased_ = Bind(self, std::move(callback));
        return self;
    }

    /**
     * @brief Ustawia reakcję na kółko myszy, gdy kursor jest nad obiektem.
     * @param callback Funkcja otrzymująca obiekt i ruch kółka (dodatni w górę).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnScroll(this Self& self, Callback<Self, float> callback) {
        self.scroll_ = [&self, fn = std::move(callback)](float wheel) { fn(self, wheel); };
        return self;
    }

    /**
     * @brief Ustawia reakcję na rozpoczęcie przeciągania (lewy przycisk wciśnięty na obiekcie).
     * @param callback Funkcja otrzymująca obiekt.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnDragStart(this Self& self, Callback<Self> callback) {
        self.dragStart_ = Bind(self, std::move(callback));
        return self;
    }

    /**
     * @brief Ustawia reakcję na ruch kursora podczas przeciągania.
     * @details Przeciąganie trwa od wciśnięcia lewego przycisku na obiekcie do jego puszczenia,
     * także gdy kursor wyjdzie poza obiekt. Przesunięcie jest w układzie rodzica, więc
     * `self.Translate(delta)` przesuwa obiekt dokładnie za kursorem, nawet w obróconej grupie.
     * @code{.cpp}
     * scene.Spawn<shapes::Rect>().Size({80, 50}).Fill()
     *     .OnDrag([](shapes::Rect& self, Vector2 delta) { self.Translate(delta); });
     * @endcode
     * @param callback Funkcja otrzymująca obiekt i przesunięcie kursora od poprzedniej klatki.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnDrag(this Self& self, Callback<Self, Vector2> callback) {
        self.drag_ = [&self, fn = std::move(callback)](Vector2 delta) { fn(self, delta); };
        return self;
    }

    /**
     * @brief Ustawia reakcję na zakończenie przeciągania (puszczenie lewego przycisku).
     * @param callback Funkcja otrzymująca obiekt.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnDragEnd(this Self& self, Callback<Self> callback) {
        self.dragEnd_ = Bind(self, std::move(callback));
        return self;
    }

    /** @} */

  protected:
    /**
     * @brief Własna logika klasy pochodnej, wywoływana co klatkę.
     * @details Miejsce na zachowanie komponentu (np. sterowanie graczem). Wywoływana po
     * zdarzeniach myszy i klawiatury, a przed callbackiem OnUpdate() - scena może więc
     * dołożyć własną logikę przez OnUpdate() bez nadpisywania zachowania klasy.
     * @param dt Czas od poprzedniej klatki w sekundach.
     * @param input Stan klawiatury i myszy.
     */
    virtual void Tick([[maybe_unused]] float dt, [[maybe_unused]] const Input& input) {}

    /**
     * @brief Własna logika klasy pochodnej wywoływana w każdym kroku stałym.
     * @details Odpowiednik _physics_process z Godota: miejsce na ruch postaci
     * (CharacterBody::MoveAndSlide()). Wywoływana przed callbackiem OnFixedUpdate().
     * @param step Stały krok w sekundach (Engine::FixedStep()).
     * @param input Stan klawiatury i myszy.
     */
    virtual void FixedTick([[maybe_unused]] float step, [[maybe_unused]] const Input& input) {}

    std::function<void(float)> update_;      /**< Callback wykonywany co klatkę. */
    std::function<void(float)> fixedUpdate_; /**< Callback wykonywany w kroku stałym. */
    std::function<void()> click_;            /**< Reakcja na kliknięcie. */
    std::function<void()> mouseEnter_;       /**< Reakcja na wejście kursora. */
    std::function<void()> mouseLeave_;       /**< Reakcja na wyjście kursora. */
    /** Reakcje na naciśnięcie klawiszy (kod klawisza, funkcja). */
    std::vector<std::pair<int, std::function<void()>>> keys_;
    /** Reakcje na puszczenie klawiszy (kod klawisza, funkcja). */
    std::vector<std::pair<int, std::function<void()>>> keysReleased_;
    std::function<void()> mouseReleased_; /**< Reakcja na puszczenie przycisku nad obiektem. */
    std::function<void(float)> scroll_;   /**< Reakcja na kółko myszy nad obiektem. */
    std::function<void()> dragStart_;     /**< Początek przeciągania. */
    std::function<void(Vector2)> drag_;   /**< Ruch podczas przeciągania. */
    std::function<void()> dragEnd_;       /**< Koniec przeciągania. */

    /**
     * @brief Zamienia ruch kursora (w układzie ekranu) na przesunięcie dla OnDrag().
     * @details Domyślnie zwraca różnicę na ekranie. Entity przelicza ją na układ rodzica.
     * @param from Poprzednia pozycja kursora.
     * @param to Bieżąca pozycja kursora.
     * @return Przesunięcie przekazywane do OnDrag().
     */
    virtual Vector2 DragDelta(Vector2 from, Vector2 to) const {
        return {to.x - from.x, to.y - from.y};
    }

  private:
    friend class Entity;        // wywołuje HandleEvents() i HandleFixed()
    friend class PointerSystem; // pyta Handles*(), żeby wybrać odbiorcę zdarzenia myszy

    /**
     * @brief Opakowuje funkcję zdarzenia tak, by otrzymywała obiekt jako jego konkretny typ.
     * @param self Obiekt, którego dotyczy zdarzenie.
     * @param callback Funkcja zdarzenia.
     * @return Funkcja bez argumentów do zapisania w polu zdarzenia.
     */
    template <typename Self>
    static std::function<void()> Bind(Self& self, std::function<void(Self&)> callback) {
        return [&self, fn = std::move(callback)] { fn(self); };
    }

    /**
     * @name Czy obiekt obsługuje dane zdarzenie myszy
     * Obiekt bez zdarzenia przepuszcza je do przodka albo obiektów pod spodem - na tej
     * podstawie PointerSystem wybiera odbiorcę (patrz @ref entity_trafienia).
     */
    /** @{ */
    bool HandlesClick() const { return static_cast<bool>(click_); }
    bool HandlesMouseReleased() const { return static_cast<bool>(mouseReleased_); }
    bool HandlesScroll() const { return static_cast<bool>(scroll_); }
    bool HandlesDrag() const { return dragStart_ || drag_ || dragEnd_; }
    /** @} */

    /**
     * @brief Wywołuje callbacki myszy i klawiatury, Tick() i callback aktualizacji.
     * @param dt Czas od poprzedniej klatki w sekundach.
     * @param input Stan wejścia.
     * @param pointer Zdarzenia myszy przydzielone temu obiektowi w tej klatce.
     */
    void HandleEvents(float dt, const Input& input, const PointerEvents& pointer);

    /**
     * @brief Wywołuje FixedTick() i callback kroku stałego.
     * @param step Stały krok w sekundach.
     * @param input Stan wejścia.
     */
    void HandleFixed(float step, const Input& input) {
        FixedTick(step, input);
        if (fixedUpdate_) fixedUpdate_(step);
    }

    /** @brief Prowadzi trwające przeciąganie: OnDrag() przy ruchu, OnDragEnd() po puszczeniu. */
    void HandleDrag(const Input& input);

    bool hovered_ = false;  /**< Ostatni zapamiętany stan najechania kursora. */
    bool dragging_ = false; /**< Czy trwa przeciąganie. */
    Vector2 dragFrom_{};    /**< Pozycja kursora w poprzedniej klatce przeciągania. */
};
