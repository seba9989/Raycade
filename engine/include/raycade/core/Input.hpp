/**
 * @file Input.hpp
 * @brief Klasa Input - odczyt klawiatury, myszy, padów i dotyku z możliwością ich wyłączenia.
 */
#pragma once

#include <raylib.h>

#include <string>

/**
 * @brief Odczyt stanu urządzeń wejścia przez raylib.
 * @details Każde urządzenie (klawiatura, mysz, pady, dotyk) można wyłączyć. Wyłączone
 * urządzenie zachowuje się tak, jakby nic nie było wciśnięte: zapytania o wciśnięcie zwracają
 * false, zapytania „Up” - true, a pozycje i ruchy - {0, 0}.
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
    bool KeyDown(int key) const { return keyboard_ && ::IsKeyDown(key); }

    /**
     * @brief Sprawdza, czy klawisz nie jest wciśnięty.
     * @param key Kod klawisza raylib.
     * @return true także przy wyłączonej klawiaturze.
     */
    bool KeyUp(int key) const { return !keyboard_ || ::IsKeyUp(key); }

    /**
     * @brief Sprawdza naciśnięcie klawisza w bieżącej klatce.
     * @param key Kod klawisza raylib.
     * @return Stan naciśnięcia.
     */
    bool KeyPressed(int key) const { return keyboard_ && ::IsKeyPressed(key); }

    /**
     * @brief Sprawdza powtórzenie naciśnięcia przy przytrzymanym klawiszu (autorepeat systemu).
     * @param key Kod klawisza raylib.
     * @return Czy w tej klatce nastąpiło powtórzenie.
     */
    bool KeyPressedRepeat(int key) const { return keyboard_ && ::IsKeyPressedRepeat(key); }

    /**
     * @brief Sprawdza puszczenie klawisza w bieżącej klatce.
     * @param key Kod klawisza raylib.
     * @return Stan puszczenia.
     */
    bool KeyReleased(int key) const { return keyboard_ && ::IsKeyReleased(key); }

    /**
     * @brief Pobiera następny klawisz z kolejki naciśnięć tej klatki.
     * @details Każde wywołanie zdejmuje jeden klawisz - wywołuj w pętli aż do 0.
     * @return Kod klawisza albo 0, gdy kolejka jest pusta.
     */
    int NextKeyPressed() const { return keyboard_ ? ::GetKeyPressed() : 0; }

    /**
     * @brief Pobiera następny wpisany znak (Unicode) z kolejki tej klatki.
     * @details Do pól tekstowych: uwzględnia układ klawiatury i Shift. Wywołuj w pętli aż do 0.
     * @return Kod znaku Unicode albo 0, gdy kolejka jest pusta.
     */
    int NextCharPressed() const { return keyboard_ ? ::GetCharPressed() : 0; }

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
    Vector2 MouseDelta() const { return mouse_ ? ::GetMouseDelta() : Vector2{0, 0}; }

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
        return touch_ ? ::GetTouchPosition(index) : Vector2{0, 0};
    }

    /**
     * @brief Pobiera identyfikator punktu dotyku (stały, dopóki palec jest na ekranie).
     * @param index Indeks punktu.
     * @return Identyfikator albo -1.
     */
    int TouchPointId(int index) const { return touch_ ? ::GetTouchPointId(index) : -1; }

    /** @} */

  private:
    bool keyboard_ = true; /**< Czy odczytywać klawiaturę. */
    bool mouse_ = true;    /**< Czy odczytywać mysz. */
    bool gamepad_ = true;  /**< Czy odczytywać pady. */
    bool touch_ = true;    /**< Czy odczytywać dotyk. */
};
