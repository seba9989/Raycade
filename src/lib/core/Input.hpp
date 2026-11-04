/**
 * @file Input.hpp
 * @brief Klasa Input - odczyt klawiatury i myszy z możliwością ich wyłączenia.
 */
#pragma once

#include <raylib.h>

/**
 * @brief Dostęp do stanu klawiatury i myszy przez raylib.
 * @details Po wyłączeniu urządzenia zapytania zwracają false albo pozycję {0, 0}.
 */
class Input {
  public:
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
     * @brief Sprawdza, czy klawiatura jest włączona.
     * @return Stan ustawienia.
     */
    bool KeyboardEnabled() const { return keyboard_; }

    /**
     * @brief Sprawdza, czy mysz jest włączona.
     * @return Stan ustawienia.
     */
    bool MouseEnabled() const { return mouse_; }

    /**
     * @brief Sprawdza, czy klawisz jest wciśnięty.
     * @param key Kod klawisza raylib.
     * @return Stan klawisza.
     */
    bool KeyDown(int key) const { return keyboard_ && ::IsKeyDown(key); }

    /**
     * @brief Sprawdza naciśnięcie klawisza w bieżącej klatce.
     * @param key Kod klawisza raylib.
     * @return Stan naciśnięcia.
     */
    bool KeyPressed(int key) const { return keyboard_ && ::IsKeyPressed(key); }

    /**
     * @brief Sprawdza, czy przycisk myszy jest wciśnięty.
     * @param button Kod przycisku raylib.
     * @return Stan przycisku.
     */
    bool MouseDown(int button) const { return mouse_ && ::IsMouseButtonDown(button); }

    /**
     * @brief Sprawdza naciśnięcie przycisku myszy w bieżącej klatce.
     * @param button Kod przycisku raylib.
     * @return Stan naciśnięcia.
     */
    bool MousePressed(int button) const { return mouse_ && ::IsMouseButtonPressed(button); }

    /**
     * @brief Pobiera pozycję kursora.
     * @return Pozycja w oknie albo {0, 0}, gdy mysz jest wyłączona.
     */
    Vector2 MousePosition() const { return mouse_ ? ::GetMousePosition() : Vector2{0, 0}; }

  private:
    bool keyboard_ = true; /**< Czy odczytywać klawiaturę. */
    bool mouse_ = true;    /**< Czy odczytywać mysz. */
};
