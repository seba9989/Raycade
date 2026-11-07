/**
 * @file InputActions.hpp
 * @brief Akcje i osie wejścia: nazwy zamiast konkretnych klawiszy i przycisków.
 */
#pragma once

#include <map>
#include <string>
#include <vector>

/**
 * @brief Jedno źródło akcji: klawisz, przycisk myszy, przycisk pada, wychylenie osi pada
 * albo gest (dotyk lub mysz).
 * @details Tworzy się je funkcjami z przestrzeni nazw @ref bind.
 */
struct InputBinding {
    /** @brief Rodzaj urządzenia. */
    enum class Kind { Key, Mouse, PadButton, PadAxis, TouchGesture };

    Kind kind = Kind::Key; /**< Rodzaj urządzenia. */
    int code = 0;    /**< KEY_*, MOUSE_BUTTON_*, GAMEPAD_BUTTON_*, GAMEPAD_AXIS_* albo GESTURE_*. */
    int gamepad = 0; /**< Numer pada. */
    float direction = 1.0f; /**< Dla osi jako przycisku: +1 albo -1 (kierunek wychylenia). */
    float threshold = 0.5f; /**< Dla osi: wychylenie, od którego liczy się jako wciśnięta. */
};

/**
 * @brief Funkcje tworzące powiązania akcji.
 * @code{.cpp}
 * engine.BindAction("skok", {bind::Key(KEY_SPACE), bind::Pad(GAMEPAD_BUTTON_RIGHT_FACE_DOWN)});
 * @endcode
 */
namespace bind {

/** @brief Klawisz klawiatury (KEY_*). */
inline InputBinding Key(int key) {
    return {InputBinding::Kind::Key, key};
}

/** @brief Przycisk myszy (MOUSE_BUTTON_*). */
inline InputBinding Mouse(int button) {
    return {InputBinding::Kind::Mouse, button};
}

/** @brief Przycisk pada (GAMEPAD_BUTTON_*). */
inline InputBinding Pad(int button, int gamepad = 0) {
    return {InputBinding::Kind::PadButton, button, gamepad};
}

/**
 * @brief Wychylenie osi pada w jedną stronę, traktowane jak przycisk (np. gałka w lewo).
 * @param axis Oś (GAMEPAD_AXIS_*).
 * @param direction +1 albo -1.
 * @param threshold Wychylenie, od którego liczy się jako wciśnięta.
 * @param gamepad Numer pada.
 */
inline InputBinding PadAxis(int axis, float direction, float threshold = 0.5f, int gamepad = 0) {
    return {InputBinding::Kind::PadAxis, axis, gamepad, direction, threshold};
}

/**
 * @brief Gest raylib (GESTURE_TAP, GESTURE_SWIPE_LEFT, GESTURE_HOLD...).
 * @details Gest trwa, dopóki raylib go zgłasza (Down), a Pressed/Released są liczone ze zmiany
 * między klatkami - jak przy osiach pada. Na komputerze gesty działają też myszą.
 * @code{.cpp}
 * engine.BindAction("skok", {bind::Key(KEY_SPACE), bind::Gesture(GESTURE_SWIPE_UP)});
 * @endcode
 * @param gesture Gest (GESTURE_*).
 */
inline InputBinding Gesture(int gesture) {
    return {InputBinding::Kind::TouchGesture, gesture};
}

} // namespace bind

/**
 * @brief Definicja osi: klawisze/przyciski na minus i na plus oraz osie analogowe.
 */
struct InputAxis {
    std::vector<InputBinding> negative; /**< Dają -1, gdy wciśnięte. */
    std::vector<InputBinding> positive; /**< Dają +1, gdy wciśnięte. */
    /** Osie analogowe pada (GAMEPAD_AXIS_*), dodawane z martwą strefą. */
    std::vector<InputBinding> analog;
    float deadzone = 0.15f; /**< Wychylenia mniejsze od tej wartości są pomijane. */
};

/**
 * @brief Zbiór akcji i osi gry (wspólny dla wszystkich kopii Input).
 */
struct InputActionMap {
    std::map<std::string, std::vector<InputBinding>> actions; /**< Akcje według nazwy. */
    std::map<std::string, InputAxis> axes;                    /**< Osie według nazwy. */
    /** Stan osi pada i gestów z poprzedniej klatki (do Pressed/Released dla PadAxis i Gesture). */
    std::map<std::string, bool> analogDown;
};
