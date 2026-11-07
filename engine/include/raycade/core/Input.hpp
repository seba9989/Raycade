/**
 * @file Input.hpp
 * @brief Klasa Input - odczyt klawiatury, myszy, padów i dotyku z możliwością ich wyłączenia.
 */
#pragma once

#include "InputActions.hpp"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

/**
 * @brief Odczyt stanu urządzeń wejścia przez raylib.
 * @details Każde urządzenie (klawiatura, mysz, pady, dotyk) można wyłączyć. Wyłączone
 * urządzenie zachowuje się tak, jakby nic nie było wciśnięte: zapytania o wciśnięcie zwracają
 * false, zapytania „Up” - true, a pozycje i ruchy - {0, 0}.
 *
 * Gdy obiekt z fokusem przejmuje klawiaturę (np. ui::TextField), wszyscy inni widzą ją jak
 * wyłączoną - KeyboardCaptured() mówi, czy tak jest w tej klatce.
 *
 * Akcje i osie (ActionPressed(), Axis()) to nazwy zamiast konkretnych klawiszy - patrz
 * Engine::BindAction() i Engine::BindAxis().
 *
 * Klasa tylko odczytuje stan. Sterowanie (pozycja i wygląd kursora, wibracja pada) jest
 * w Engine. Obiekty sceny dostają Input w Tick(), a z dowolnego miejsca jest dostępny przez
 * Engine::Get().GetInput().
 */
class Input {
  public:
    /** @name Włączanie urządzeń */
    /** @{ */

    /**
     * @brief Włącza lub wyłącza obsługę klawiatury.
     * @param value Czy włączyć klawiaturę.
     * @return Ten sam obiekt.
     */
    Input& Keyboard(bool value = true) {
        keyboard_ = value;
        return *this;
    }

    /**
     * @brief Włącza lub wyłącza obsługę myszy.
     * @param value Czy włączyć mysz.
     * @return Ten sam obiekt.
     */
    Input& Mouse(bool value = true) {
        mouse_ = value;
        return *this;
    }

    /**
     * @brief Włącza lub wyłącza obsługę padów.
     * @param value Czy włączyć pady.
     * @return Ten sam obiekt.
     */
    Input& Gamepad(bool value = true) {
        gamepad_ = value;
        return *this;
    }

    /**
     * @brief Włącza lub wyłącza obsługę ekranu dotykowego.
     * @param value Czy włączyć dotyk.
     * @return Ten sam obiekt.
     */
    Input& Touch(bool value = true) {
        touch_ = value;
        return *this;
    }

    /** @brief Sprawdza, czy klawiatura jest włączona. */
    bool KeyboardEnabled() const { return keyboard_; }
    /**
     * @brief Sprawdza, czy klawiaturę przejął obiekt z fokusem (np. pole tekstowe).
     * @details Wtedy zapytania o klawisze, akcje z klawiatury i wpisywane znaki zwracają
     * „nic nie wciśnięte” - poza samym obiektem z fokusem.
     */
    bool KeyboardCaptured() const { return captured_; }
    /** @brief Sprawdza, czy mysz jest włączona. */
    bool MouseEnabled() const { return mouse_; }
    /** @brief Sprawdza, czy pady są włączone. */
    bool GamepadEnabled() const { return gamepad_; }
    /** @brief Sprawdza, czy dotyk jest włączony. */
    bool TouchEnabled() const { return touch_; }

    /** @} */

    /** @name Klawiatura */
    /** @{ */

    /**
     * @brief Sprawdza, czy klawisz jest wciśnięty.
     * @param key Kod klawisza raylib (KEY_*).
     * @return Stan klawisza.
     */
    bool KeyDown(int key) const { return KeyboardActive() && ::IsKeyDown(key); }

    /**
     * @brief Sprawdza, czy klawisz nie jest wciśnięty.
     * @param key Kod klawisza raylib.
     * @return true także przy wyłączonej klawiaturze.
     */
    bool KeyUp(int key) const { return !KeyboardActive() || ::IsKeyUp(key); }

    /**
     * @brief Sprawdza naciśnięcie klawisza w bieżącej klatce.
     * @param key Kod klawisza raylib.
     * @return Stan naciśnięcia.
     */
    bool KeyPressed(int key) const { return KeyboardActive() && ::IsKeyPressed(key); }

    /**
     * @brief Sprawdza powtórzenie naciśnięcia przy przytrzymanym klawiszu (autorepeat systemu).
     * @param key Kod klawisza raylib.
     * @return Czy w tej klatce nastąpiło powtórzenie.
     */
    bool KeyPressedRepeat(int key) const { return KeyboardActive() && ::IsKeyPressedRepeat(key); }

    /**
     * @brief Sprawdza puszczenie klawisza w bieżącej klatce.
     * @param key Kod klawisza raylib.
     * @return Stan puszczenia.
     */
    bool KeyReleased(int key) const { return KeyboardActive() && ::IsKeyReleased(key); }

    /**
     * @brief Pobiera następny klawisz z kolejki naciśnięć tej klatki.
     * @details Każde wywołanie zdejmuje jeden klawisz - wywołuj w pętli aż do 0.
     * @return Kod klawisza albo 0, gdy kolejka jest pusta.
     */
    int NextKeyPressed() const { return KeyboardActive() ? ::GetKeyPressed() : 0; }

    /**
     * @brief Pobiera następny wpisany znak (Unicode) z kolejki tej klatki.
     * @details Do pól tekstowych: uwzględnia układ klawiatury i Shift. Wywołuj w pętli aż do 0.
     * @return Kod znaku Unicode albo 0, gdy kolejka jest pusta.
     */
    int NextCharPressed() const { return KeyboardActive() ? ::GetCharPressed() : 0; }

    /** @} */

    /** @name Mysz */
    /** @{ */

    /**
     * @brief Sprawdza, czy przycisk myszy jest wciśnięty.
     * @param button Kod przycisku raylib (MOUSE_BUTTON_*).
     * @return Stan przycisku.
     */
    bool MouseDown(int button) const { return mouse_ && ::IsMouseButtonDown(button); }

    /**
     * @brief Sprawdza, czy przycisk myszy nie jest wciśnięty.
     * @param button Kod przycisku raylib.
     * @return true także przy wyłączonej myszy.
     */
    bool MouseUp(int button) const { return !mouse_ || ::IsMouseButtonUp(button); }

    /**
     * @brief Sprawdza naciśnięcie przycisku myszy w bieżącej klatce.
     * @param button Kod przycisku raylib.
     * @return Stan naciśnięcia.
     */
    bool MousePressed(int button) const { return mouse_ && ::IsMouseButtonPressed(button); }

    /**
     * @brief Sprawdza puszczenie przycisku myszy w bieżącej klatce.
     * @param button Kod przycisku raylib.
     * @return Stan puszczenia.
     */
    bool MouseReleased(int button) const { return mouse_ && ::IsMouseButtonReleased(button); }

    /**
     * @brief Pobiera pozycję kursora.
     * @return Pozycja w oknie albo {0, 0}, gdy mysz jest wyłączona.
     */
    Vector2 MousePosition() const { return mouse_ ? ::GetMousePosition() : Vector2{0, 0}; }

    /**
     * @brief Pobiera przesunięcie kursora od poprzedniej klatki.
     * @return Przesunięcie w pikselach albo {0, 0}, gdy mysz jest wyłączona.
     */
    Vector2 MouseDelta() const {
        if (!mouse_) return {0, 0};
        // raylib skaluje pozycję kursora (SetMouseScale), ale nie jego przesunięcie
        const Vector2 delta = ::GetMouseDelta();
        return {delta.x * scale_.x, delta.y * scale_.y};
    }

    /**
     * @brief Pobiera ruch kółka myszy w bieżącej klatce (większa z osi).
     * @return Ruch kółka; dodatni w górę.
     */
    float MouseWheel() const { return mouse_ ? ::GetMouseWheelMove() : 0.0f; }

    /**
     * @brief Pobiera ruch kółka myszy w obu osiach (np. touchpad, kółko poziome).
     * @return Ruch w osi X i Y.
     */
    Vector2 MouseWheelV() const { return mouse_ ? ::GetMouseWheelMoveV() : Vector2{0, 0}; }

    /** @} */

    /** @name Pady */
    /** @{ */

    /**
     * @brief Sprawdza, czy pad o podanym numerze jest podłączony.
     * @param gamepad Numer pada (od 0).
     * @return Dostępność pada.
     */
    bool GamepadAvailable(int gamepad) const { return gamepad_ && ::IsGamepadAvailable(gamepad); }

    /**
     * @brief Pobiera nazwę pada.
     * @param gamepad Numer pada.
     * @return Nazwa albo pusty napis, gdy pad jest niedostępny.
     */
    std::string GamepadName(int gamepad) const {
        if (!GamepadAvailable(gamepad)) return {};
        const char* name = ::GetGamepadName(gamepad);
        return name ? name : "";
    }

    /**
     * @brief Sprawdza, czy przycisk pada jest wciśnięty.
     * @param gamepad Numer pada.
     * @param button Kod przycisku raylib (GAMEPAD_BUTTON_*).
     * @return Stan przycisku.
     */
    bool GamepadDown(int gamepad, int button) const {
        return gamepad_ && ::IsGamepadButtonDown(gamepad, button);
    }

    /**
     * @brief Sprawdza, czy przycisk pada nie jest wciśnięty.
     * @return true także przy wyłączonych padach.
     */
    bool GamepadUp(int gamepad, int button) const {
        return !gamepad_ || ::IsGamepadButtonUp(gamepad, button);
    }

    /**
     * @brief Sprawdza naciśnięcie przycisku pada w bieżącej klatce.
     * @return Stan naciśnięcia.
     */
    bool GamepadPressed(int gamepad, int button) const {
        return gamepad_ && ::IsGamepadButtonPressed(gamepad, button);
    }

    /**
     * @brief Sprawdza puszczenie przycisku pada w bieżącej klatce.
     * @return Stan puszczenia.
     */
    bool GamepadReleased(int gamepad, int button) const {
        return gamepad_ && ::IsGamepadButtonReleased(gamepad, button);
    }

    /**
     * @brief Pobiera ostatnio naciśnięty przycisk dowolnego pada.
     * @return Kod przycisku albo GAMEPAD_BUTTON_UNKNOWN (0).
     */
    int LastGamepadButton() const { return gamepad_ ? ::GetGamepadButtonPressed() : 0; }

    /**
     * @brief Pobiera liczbę osi pada.
     * @param gamepad Numer pada.
     * @return Liczba osi albo 0.
     */
    int GamepadAxisCount(int gamepad) const {
        return gamepad_ ? ::GetGamepadAxisCount(gamepad) : 0;
    }

    /**
     * @brief Pobiera wychylenie osi pada.
     * @param gamepad Numer pada.
     * @param axis Kod osi raylib (GAMEPAD_AXIS_*).
     * @return Wartość od -1 do 1 (spusty: od -1 do 1, zwolniony = -1).
     */
    float GamepadAxis(int gamepad, int axis) const {
        return gamepad_ ? ::GetGamepadAxisMovement(gamepad, axis) : 0.0f;
    }

    /** @} */

    /** @name Dotyk */
    /** @{ */

    /**
     * @brief Pobiera liczbę punktów dotyku.
     * @return Liczba palców na ekranie albo 0.
     */
    int TouchPointCount() const { return touch_ ? ::GetTouchPointCount() : 0; }

    /**
     * @brief Pobiera pozycję punktu dotyku.
     * @param index Indeks punktu (od 0 do TouchPointCount() - 1).
     * @return Pozycja w oknie albo {0, 0}.
     */
    Vector2 TouchPosition(int index = 0) const {
        if (!touch_) return {0, 0};
        // raylib nie przelicza dotyku przez przesunięcie i skalę ekranu - robimy to tutaj
        const Vector2 position = ::GetTouchPosition(index);
        return {(position.x + offset_.x) * scale_.x, (position.y + offset_.y) * scale_.y};
    }

    /**
     * @brief Pobiera identyfikator punktu dotyku (stały, dopóki palec jest na ekranie).
     * @param index Indeks punktu.
     * @return Identyfikator albo -1.
     */
    int TouchPointId(int index) const { return touch_ ? ::GetTouchPointId(index) : -1; }

    /** @} */

    /**
     * @name Akcje i osie
     * Powiązania ustawia się przez Engine::BindAction() i Engine::BindAxis(). Akcje respektują
     * wyłączniki urządzeń i przejęcie klawiatury przez pole tekstowe. Nieznana nazwa daje
     * false albo 0.
     */
    /** @{ */

    /**
     * @brief Sprawdza, czy któreś źródło akcji jest wciśnięte.
     * @param action Nazwa akcji, np. "skok".
     * @return Stan akcji.
     */
    bool ActionDown(const std::string& action) const {
        const auto* bindings = Find(actions_->actions, action);
        if (!bindings) return false;
        return std::any_of(bindings->begin(), bindings->end(),
                           [&](const InputBinding& b) { return BindingDown(b); });
    }

    /**
     * @brief Sprawdza, czy akcję naciśnięto w tej klatce.
     * @details Dla wychyleń osi pada (bind::PadAxis) liczy się przekroczenie progu w tej klatce.
     * @param action Nazwa akcji.
     * @return Stan naciśnięcia.
     */
    bool ActionPressed(const std::string& action) const {
        const auto* bindings = Find(actions_->actions, action);
        if (!bindings) return false;
        for (const InputBinding& b : *bindings)
            if (b.kind != InputBinding::Kind::PadAxis && BindingPressed(b)) return true;
        return AnalogDown(*bindings) && !WasAnalogDown(action);
    }

    /**
     * @brief Sprawdza, czy akcję puszczono w tej klatce (żadne źródło nie jest już wciśnięte).
     * @param action Nazwa akcji.
     * @return Stan puszczenia.
     */
    bool ActionReleased(const std::string& action) const {
        const auto* bindings = Find(actions_->actions, action);
        if (!bindings || ActionDown(action)) return false;
        for (const InputBinding& b : *bindings)
            if (b.kind != InputBinding::Kind::PadAxis && BindingReleased(b)) return true;
        return WasAnalogDown(action) && !AnalogDown(*bindings);
    }

    /**
     * @brief Pobiera wartość osi.
     * @details Suma: -1 za wciśnięte źródło ujemne, +1 za dodatnie i wychylenie osi analogowych
     * (poza martwą strefą), obcięta do przedziału [-1, 1].
     * @param axis Nazwa osi, np. "ruch_x".
     * @return Wartość od -1 do 1.
     */
    float Axis(const std::string& axis) const {
        const auto* definition = Find(actions_->axes, axis);
        if (!definition) return 0.0f;
        float value = 0.0f;
        for (const InputBinding& b : definition->negative)
            if (BindingDown(b)) value -= 1.0f;
        for (const InputBinding& b : definition->positive)
            if (BindingDown(b)) value += 1.0f;
        for (const InputBinding& b : definition->analog) {
            const float v =
                gamepad_ ? ::GetGamepadAxisMovement(b.gamepad, b.code) * b.direction : 0.0f;
            if (std::fabs(v) > definition->deadzone) value += v;
        }
        return std::clamp(value, -1.0f, 1.0f);
    }

    /**
     * @brief Łączy dwie osie w kierunek o długości najwyżej 1 (np. ruch po skosie nie jest
     * szybszy).
     * @param x Nazwa osi poziomej.
     * @param y Nazwa osi pionowej.
     * @return Wektor kierunku.
     */
    Vector2 AxisVector(const std::string& x, const std::string& y) const {
        Vector2 v{Axis(x), Axis(y)};
        const float length = std::sqrt(v.x * v.x + v.y * v.y);
        if (length > 1.0f) v = {v.x / length, v.y / length};
        return v;
    }

    /**
     * @brief Zwraca powiązania akcji (np. do wyświetlenia w ustawieniach sterowania).
     * @param action Nazwa akcji.
     * @return Lista powiązań (pusta dla nieznanej akcji).
     */
    std::vector<InputBinding> ActionBindings(const std::string& action) const {
        const auto* bindings = Find(actions_->actions, action);
        return bindings ? *bindings : std::vector<InputBinding>{};
    }

    /** @} */

  private:
    friend class Engine;
    friend class Entity;
    friend class Scene;

    /** @brief Klawiatura włączona i nieprzejęta przez obiekt z fokusem. */
    bool KeyboardActive() const { return keyboard_ && !captured_; }

    /** @brief Szuka wpisu w mapie akcji albo osi. */
    template <typename Map>
    static const typename Map::mapped_type* Find(const Map& map, const std::string& name) {
        const auto it = map.find(name);
        return it == map.end() ? nullptr : &it->second;
    }

    /** @brief Czy pojedyncze źródło jest wciśnięte. */
    bool BindingDown(const InputBinding& b) const {
        switch (b.kind) {
        case InputBinding::Kind::Key:
            return KeyDown(b.code);
        case InputBinding::Kind::Mouse:
            return MouseDown(b.code);
        case InputBinding::Kind::PadButton:
            return GamepadDown(b.gamepad, b.code);
        case InputBinding::Kind::PadAxis:
            return gamepad_ &&
                   ::GetGamepadAxisMovement(b.gamepad, b.code) * b.direction > b.threshold;
        }
        return false;
    }

    /** @brief Czy źródło (poza osią pada) naciśnięto w tej klatce. */
    bool BindingPressed(const InputBinding& b) const {
        switch (b.kind) {
        case InputBinding::Kind::Key:
            return KeyPressed(b.code);
        case InputBinding::Kind::Mouse:
            return MousePressed(b.code);
        case InputBinding::Kind::PadButton:
            return GamepadPressed(b.gamepad, b.code);
        case InputBinding::Kind::PadAxis:
            return false;
        }
        return false;
    }

    /** @brief Czy źródło (poza osią pada) puszczono w tej klatce. */
    bool BindingReleased(const InputBinding& b) const {
        switch (b.kind) {
        case InputBinding::Kind::Key:
            return KeyReleased(b.code);
        case InputBinding::Kind::Mouse:
            return MouseReleased(b.code);
        case InputBinding::Kind::PadButton:
            return GamepadReleased(b.gamepad, b.code);
        case InputBinding::Kind::PadAxis:
            return false;
        }
        return false;
    }

    /** @brief Czy któraś oś pada z listy jest teraz wychylona ponad próg. */
    bool AnalogDown(const std::vector<InputBinding>& bindings) const {
        return std::any_of(bindings.begin(), bindings.end(), [&](const InputBinding& b) {
            return b.kind == InputBinding::Kind::PadAxis && BindingDown(b);
        });
    }

    /** @brief Stan wychyleń osi pada akcji z poprzedniej klatki. */
    bool WasAnalogDown(const std::string& action) const {
        const auto it = actions_->analogDown.find(action);
        return it != actions_->analogDown.end() && it->second;
    }

    /** @brief Zapamiętuje stan wychyleń osi pada (wywoływane przez Engine na końcu klatki). */
    void UpdateActionState() {
        for (const auto& [name, bindings] : actions_->actions)
            actions_->analogDown[name] = AnalogDown(bindings);
    }

    /**
     * @brief Ustawia przeliczenie współrzędnych okna na współrzędne gry (skalowanie ekranu).
     * @details Wywoływane przez Engine co klatkę przy włączonym ScaleMode(). Przekazuje to samo
     * do raylib (SetMouseOffset/SetMouseScale), więc MousePosition() też jest przeliczone.
     * @param offset Przesunięcie dodawane do pozycji w oknie.
     * @param scale Skala mnożona po przesunięciu.
     */
    void Transform(Vector2 offset, Vector2 scale) {
        offset_ = offset;
        scale_ = scale;
        ::SetMouseOffset(static_cast<int>(offset.x), static_cast<int>(offset.y));
        ::SetMouseScale(scale.x, scale.y);
    }

    Vector2 offset_{0, 0};  /**< Przesunięcie okno → gra. */
    Vector2 scale_{1, 1};   /**< Skala okno → gra. */
    bool keyboard_ = true;  /**< Czy odczytywać klawiaturę. */
    bool mouse_ = true;     /**< Czy odczytywać mysz. */
    bool gamepad_ = true;   /**< Czy odczytywać pady. */
    bool touch_ = true;     /**< Czy odczytywać dotyk. */
    bool captured_ = false; /**< Czy klawiaturę przejął obiekt z fokusem. */
    /** Akcje i osie - wspólne dla kopii Input (kopia dla obiektu z fokusem). */
    std::shared_ptr<InputActionMap> actions_ = std::make_shared<InputActionMap>();
};
