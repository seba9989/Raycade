/**
 * @file Updatable.hpp
 * @brief Aktualizacja jednego obiektu i obsługa jego zdarzeń wejścia.
 */
#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include "Input.hpp"

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
 * @details Wypełnia je Entity na podstawie trafień policzonych przez scenę (patrz
 * @ref entity_trafienia), więc obiekt nie sprawdza kursora sam.
 */
struct PointerEvents {
    bool hovered =
        false; /**< Kursor nad obiektem (albo nad jego potomkiem) i nic go nie zasłania. */
    bool pressed = false;   /**< Obiekt dostaje OnClick w tej klatce. */
    bool released = false;  /**< Obiekt dostaje OnMouseUp w tej klatce. */
    bool scrolled = false;  /**< Obiekt dostaje OnScroll w tej klatce. */
    bool dragStart = false; /**< Obiekt zaczyna przeciąganie w tej klatce. */
};

/**
 * @brief Obiekt aktualizowany co klatkę.
 * @details Klasa obsługuje tylko własne zdarzenia. Aktualizacja dzieci należy do Entity.
 * Domyślna implementacja Contains() nie ma obszaru reagowania na mysz.
 */
class Updatable {
  public:
    virtual ~Updatable() = default;

    /**
     * @brief Sprawdza trafienie punktem w obiekt.
     * @param point Punkt we współrzędnych ekranu.
     * @return true, gdy punkt należy do obiektu.
     */
    virtual bool Contains([[maybe_unused]] Vector2 point) const { return false; }

    /**
     * @brief Sprawdza, czy kursor jest nad obiektem.
     * @details Tylko najwyżej narysowany obiekt pod kursorem i jego przodkowie są „najechani”
     * - obiekt zasłonięty przez inny nie jest.
     */
    bool Hovered() const { return hovered_; }

    /** @brief Sprawdza, czy obiekt jest właśnie przeciągany (patrz OnDrag()). */
    bool Dragging() const { return dragging_; }

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
        self.update_ = [&self, callback = std::move(callback)](float dt) { callback(self, dt); };
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
    template <typename Self> Self& OnMouseUp(this Self& self, Callback<Self> callback) {
        self.mouseUp_ = Bind(self, std::move(callback));
        return self;
    }

    /**
     * @brief Ustawia reakcję na kółko myszy, gdy kursor jest nad obiektem.
     * @param callback Funkcja otrzymująca obiekt i ruch kółka (dodatni w górę).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnScroll(this Self& self, Callback<Self, float> callback) {
        self.scroll_ = [&self, callback = std::move(callback)](float wheel) {
            callback(self, wheel);
        };
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
        self.drag_ = [&self, callback = std::move(callback)](Vector2 delta) {
            callback(self, delta);
        };
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

    std::function<void(float)> update_; /**< Callback wykonywany co klatkę. */
    std::function<void()> click_;       /**< Reakcja na kliknięcie. */
    std::function<void()> mouseEnter_;  /**< Reakcja na wejście kursora. */
    std::function<void()> mouseLeave_;  /**< Reakcja na wyjście kursora. */
    std::vector<std::pair<int, std::function<void()>>> keys_; /**< Reakcje na klawisze. */
    /** Reakcje na puszczenie klawiszy. */
    std::vector<std::pair<int, std::function<void()>>> keysReleased_;
    std::function<void()> mouseUp_;     /**< Reakcja na puszczenie przycisku nad obiektem. */
    std::function<void(float)> scroll_; /**< Reakcja na kółko myszy nad obiektem. */
    std::function<void()> dragStart_;   /**< Początek przeciągania. */
    std::function<void(Vector2)> drag_; /**< Ruch podczas przeciągania. */
    std::function<void()> dragEnd_;     /**< Koniec przeciągania. */

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
    friend class Entity;

    /**
     * @brief Opakowuje funkcję zdarzenia tak, by otrzymywała obiekt jako jego konkretny typ.
     * @param self Obiekt, którego dotyczy zdarzenie.
     * @param callback Funkcja zdarzenia.
     * @return Funkcja bez argumentów do zapisania w polu zdarzenia.
     */
    template <typename Self>
    static std::function<void()> Bind(Self& self, std::function<void(Self&)> callback) {
        return [&self, callback = std::move(callback)] { callback(self); };
    }

    /** @name Czy obiekt obsługuje dane zdarzenie myszy (do wyboru odbiorcy przez scenę) */
    /** @{ */
    bool HandlesClick() const { return static_cast<bool>(click_); }
    bool HandlesMouseUp() const { return static_cast<bool>(mouseUp_); }
    bool HandlesScroll() const { return static_cast<bool>(scroll_); }
    bool HandlesDrag() const { return dragStart_ || drag_ || dragEnd_; }
    /** @} */

    /**
     * @brief Wywołuje callbacki myszy i klawiatury, Tick() i callback aktualizacji.
     * @param dt Czas od poprzedniej klatki w sekundach.
     * @param input Stan wejścia.
     * @param pointer Zdarzenia myszy przydzielone temu obiektowi przez scenę.
     */
    void HandleEvents(float dt, const Input& input, const PointerEvents& pointer) {
        if (pointer.hovered && !hovered_ && mouseEnter_) mouseEnter_();
        if (!pointer.hovered && hovered_ && mouseLeave_) mouseLeave_();
        hovered_ = pointer.hovered;
        if (pointer.pressed && click_) click_();
        if (pointer.dragStart && !dragging_) {
            dragging_ = true;
            dragFrom_ = input.MousePosition();
            if (dragStart_) dragStart_();
        }
        if (pointer.released && mouseUp_) mouseUp_();
        if (pointer.scrolled && scroll_) scroll_(input.MouseWheel());
        HandleDrag(input);
        for (const auto& [key, callback] : keys_)
            if (input.KeyPressed(key)) callback();
        for (const auto& [key, callback] : keysReleased_)
            if (input.KeyReleased(key)) callback();
        Tick(dt, input);
        if (update_) update_(dt);
    }

    /** @brief Prowadzi trwające przeciąganie: OnDrag() przy ruchu, OnDragEnd() po puszczeniu. */
    void HandleDrag(const Input& input) {
        if (!dragging_) return;
        // koniec także po wyłączeniu myszy - inaczej obiekt zostałby "przyklejony" do kursora
        if (!input.MouseDown(MOUSE_BUTTON_LEFT)) {
            dragging_ = false;
            if (dragEnd_) dragEnd_();
            return;
        }
        const Vector2 mouse = input.MousePosition();
        if (mouse.x == dragFrom_.x && mouse.y == dragFrom_.y) return;
        const Vector2 delta = DragDelta(dragFrom_, mouse);
        dragFrom_ = mouse;
        if (drag_) drag_(delta);
    }

    bool hovered_ = false;  /**< Ostatni zapamiętany stan najechania kursora. */
    bool dragging_ = false; /**< Czy trwa przeciąganie. */
    Vector2 dragFrom_{};    /**< Pozycja kursora w poprzedniej klatce przeciągania. */
};
