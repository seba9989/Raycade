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
 * @brief Obiekt aktualizowany co klatkę.
 * @details Klasa obsługuje tylko własne zdarzenia. Aktualizacja dzieci należy do Entity.
 * Domyślna implementacja Contains() nie ma obszaru reagowania na mysz.
 */
class Updatable {
  public:
    virtual ~Updatable() = default;

    /**
     * @brief Obsługuje zdarzenia tego obiektu.
     * @param dt Czas od poprzedniej klatki w sekundach.
     * @param input Stan klawiatury i myszy.
     */
    virtual void Update(float dt, const Input& input) { HandleEvents(dt, input); }

    /**
     * @brief Sprawdza trafienie punktem w obiekt.
     * @param point Punkt we współrzędnych ekranu.
     * @return true, gdy punkt należy do obiektu.
     */
    virtual bool Contains([[maybe_unused]] Vector2 point) const { return false; }

    /** @brief Zwraca zapamiętany stan najechania kursora. */
    bool Hovered() const { return hovered_; }

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

    /** @} */

  protected:
    std::function<void(float)> update_;                       /**< Callback wykonywany co klatkę. */
    std::function<void()> click_;                             /**< Reakcja na kliknięcie. */
    std::function<void()> mouseEnter_;                        /**< Reakcja na wejście kursora. */
    std::function<void()> mouseLeave_;                        /**< Reakcja na wyjście kursora. */
    std::vector<std::pair<int, std::function<void()>>> keys_; /**< Reakcje na klawisze. */

  private:
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

    /** @brief Wywołuje callbacki myszy, klawiatury i aktualizacji. */
    void HandleEvents(float dt, const Input& input) {
        if (input.MouseEnabled()) {
            const bool inside = Contains(input.MousePosition());
            if (inside && !hovered_ && mouseEnter_) mouseEnter_();
            if (!inside && hovered_ && mouseLeave_) mouseLeave_();
            hovered_ = inside;
            if (inside && input.MousePressed(MOUSE_BUTTON_LEFT) && click_) click_();
        }
        for (const auto& [key, callback] : keys_)
            if (input.KeyPressed(key)) callback();
        if (update_) update_(dt);
    }

    bool hovered_ = false; /**< Ostatni zapamiętany stan najechania kursora. */
};
