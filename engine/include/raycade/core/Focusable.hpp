/**
 * @file Focusable.hpp
 * @brief Klasa cechy Focusable - fokus klawiatury dla obiektów, które go potrzebują.
 */
#pragma once

#include "Updatable.hpp"

#include <algorithm>
#include <climits>
#include <functional>
#include <vector>

class Entity;
class Focusable;

/**
 * @brief Fokus klawiatury sceny (jeden na scenę).
 * @details Obiekty z cechą Focusable same dopisują się do members przy dodaniu do sceny i
 * wypisują przy zniszczeniu, więc scena nie musi przeszukiwać drzewa obiektów.
 */
struct FocusState {
    const Focusable* focused = nullptr;    /**< Obiekt z fokusem albo nullptr. */
    const Focusable* notified = nullptr;   /**< Ostatni, któremu wysłano OnFocus(). */
    std::vector<const Focusable*> members; /**< Obiekty z cechą, w kolejności dodania. */
};

/**
 * @brief Cecha obiektu, który może mieć fokus klawiatury.
 * @details Dokleja się ją do dowolnego obiektu sceny przez dziedziczenie wielokrotne - sama
 * nie jest obiektem sceny, więc zwykłe obiekty nie płacą za fokus ani pamięcią, ani API:
 * @code{.cpp}
 * class MenuButton : public shapes::Rect, public Focusable {
 *   protected:
 *     void Tick(float, const Input& input) override {
 *         if (Focused() && input.KeyPressed(KEY_ENTER)) Activate();
 *     }
 * };
 *
 * scene.Spawn<MenuButton>().Size({200, 50}).OnFocus([](MenuButton& self) { self.Paint(GOLD); });
 * @endcode
 *
 * Jak działa fokus:
 * - kliknięcie daje fokus trafionemu obiektowi z tą cechą (według zasad z
 *   @ref entity_trafienia, także przez rodzica), a kliknięcie w cokolwiek innego go zabiera,
 * - Tab i Shift+Tab przechodzą między takimi obiektami: najpierw te z TabIndex() > 0
 *   rosnąco, potem te z TabIndex() == 0 (domyślne) w kolejności dodania do sceny; obiekty z
 *   TabIndex() < 0 dostają fokus tylko kliknięciem lub z kodu
 *   (Scene::TabNavigation() wyłącza Tab w całej scenie),
 * - Focus() i Blur() zmieniają fokus z kodu, a OnFocus() / OnBlur() reagują na zmianę
 *   (na początku następnej klatki, a przy kliknięciu - w tej samej),
 * - obiekt z fokusem, którego CapturesKeyboard() zwraca true (np. ui::TextField), dostaje
 *   klawiaturę na wyłączność: inne obiekty, OnUpdate sceny i silnika oraz akcje z klawiatury
 *   widzą ją wtedy jak puszczoną (Input::KeyboardCaptured()).
 *
 * Każda scena ma własny fokus.
 */
class Focusable {
  public:
    virtual ~Focusable() {
        if (!state_) return;
        std::erase(state_->members, this);
        if (state_->focused == this) state_->focused = nullptr;
        if (state_->notified == this) state_->notified = nullptr; // bez OnBlur() na martwym
    }

    /**
     * @brief Daje obiektowi fokus klawiatury (zabierając go innym w scenie).
     * @details Wywołane przed dodaniem obiektu do sceny (np. w konstruktorze) zadziała przy
     * dodaniu.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Focus(this Self& self) {
        Focusable& focusable = self;
        if (focusable.state_)
            focusable.state_->focused = &focusable;
        else
            focusable.wantsFocus_ = true;
        return self;
    }

    /**
     * @brief Zabiera obiektowi fokus (jeśli go ma).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Blur(this Self& self) {
        Focusable& focusable = self;
        focusable.wantsFocus_ = false;
        if (focusable.Focused()) focusable.state_->focused = nullptr;
        return self;
    }

    /** @brief Sprawdza, czy obiekt ma teraz fokus. */
    bool Focused() const { return state_ && state_->focused == this; }

    /**
     * @brief Ustawia miejsce obiektu w kolejności Taba (jak tabindex w HTML).
     * @param index 0 - w kolejności dodania (domyślnie), > 0 - przed nimi, rosnąco,
     * < 0 - pomijany przez Tab.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& TabIndex(this Self& self, int index) {
        static_cast<Focusable&>(self).tabIndex_ = index;
        return self;
    }

    /** @brief Zwraca miejsce obiektu w kolejności Taba. */
    int TabIndex() const { return tabIndex_; }

    /** @brief Zwraca ten sam obiekt jako Entity (nullptr przed dodaniem do sceny). */
    const Entity* Owner() const { return owner_; }

    /**
     * @brief Ustawia reakcję na otrzymanie fokusu.
     * @param callback Funkcja otrzymująca obiekt.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnFocus(this Self& self, Callback<Self> callback) {
        static_cast<Focusable&>(self).focus_ = [&self, fn = std::move(callback)] { fn(self); };
        return self;
    }

    /**
     * @brief Ustawia reakcję na utratę fokusu.
     * @param callback Funkcja otrzymująca obiekt.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnBlur(this Self& self, Callback<Self> callback) {
        static_cast<Focusable&>(self).blur_ = [&self, fn = std::move(callback)] { fn(self); };
        return self;
    }

    /**
     * @brief Czy obiekt z fokusem zabiera klawiaturę reszcie gry.
     * @details Domyślnie nie. Pola tekstowe zwracają true, żeby wpisywane litery i cyfry nie
     * sterowały jednocześnie grą.
     */
    virtual bool CapturesKeyboard() const { return false; }

  private:
    friend class Entity;
    friend class Scene;

    /**
     * @brief Podpina cechę do fokusu sceny (wywoływane przez Entity przy dodaniu do sceny).
     * @param state Fokus sceny.
     * @param owner Ten sam obiekt jako Entity.
     */
    void AttachFocus(FocusState& state, const Entity* owner) {
        if (state_ == &state) return; // już zarejestrowany
        state_ = &state;
        owner_ = owner;
        state.members.push_back(this);
        if (wantsFocus_) {
            wantsFocus_ = false;
            state.focused = this;
        }
    }

    /**
     * @brief Zwraca obiekty, między którymi przechodzi Tab, w kolejności przechodzenia.
     * @param state Fokus sceny.
     */
    static std::vector<const Focusable*> TabOrder(const FocusState& state) {
        std::vector<const Focusable*> order;
        for (const Focusable* f : state.members)
            if (f->tabIndex_ >= 0) order.push_back(f);
        std::ranges::stable_sort(order, {}, [](const Focusable* f) {
            return f->tabIndex_ == 0 ? INT_MAX : f->tabIndex_;
        });
        return order;
    }

    FocusState* state_ = nullptr;   /**< Fokus sceny. */
    const Entity* owner_ = nullptr; /**< Ten sam obiekt jako Entity. */
    int tabIndex_ = 0;              /**< Miejsce w kolejności Taba. */
    bool wantsFocus_ = false;       /**< Focus() przed dodaniem do sceny. */
    std::function<void()> focus_;   /**< Otrzymanie fokusu. */
    std::function<void()> blur_;    /**< Utrata fokusu. */
};
