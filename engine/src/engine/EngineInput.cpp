/**
 * @file EngineInput.cpp
 * @brief Kursor, pady oraz nagrywanie i odtwarzanie wejścia.
 */

#include <raycade/Engine.hpp>

Engine& Engine::ExitKey(int key) {
    exitKey_ = key;
    // przy przejętej klawiaturze klawisz wyjścia jest wyłączony - wróci po utracie fokusu
    if (initialized_ && !input_.IsKeyboardCaptured()) ::SetExitKey(key);
    return *this;
}

Engine& Engine::MousePosition(Vector2 position) {
    if (!RequireWindow("MousePosition")) return *this;
    // pozycja podawana jest we współrzędnych gry; raylib oczekuje pikseli okna
    if (target_.IsValid()) position = input_.GameToWindow(position);
    ::SetMousePosition(static_cast<int>(position.x), static_cast<int>(position.y));
    return *this;
}

Engine& Engine::MouseCursor(int cursor) {
    if (RequireWindow("MouseCursor")) ::SetMouseCursor(cursor);
    return *this;
}

Engine& Engine::HideCursor(bool value) {
    if (!RequireWindow("HideCursor")) return *this;
    if (value)
        ::HideCursor();
    else
        ::ShowCursor();
    return *this;
}

Engine& Engine::LockCursor(bool value) {
    if (!RequireWindow("LockCursor")) return *this;
    cursorLocked_ = value;
    if (value)
        ::DisableCursor();
    else
        ::EnableCursor();
    return *this;
}

bool Engine::IsCursorHidden() const {
    return initialized_ && ::IsCursorHidden();
}

bool Engine::IsCursorOnScreen() const {
    return initialized_ && ::IsCursorOnScreen();
}

Engine& Engine::GamepadVibration(int gamepad, float left, float right, float seconds) {
    if (RequireWindow("GamepadVibration")) ::SetGamepadVibration(gamepad, left, right, seconds);
    return *this;
}

int Engine::GamepadMappings(const std::string& mappings) {
    if (!RequireWindow("GamepadMappings")) return 0;
    return ::SetGamepadMappings(mappings.c_str());
}

Engine& Engine::StartRecording() {
    if (!RequireWindow("StartRecording") || recording_) return *this;
    if (replaying_) {
        Log(LogLevel::Warning, "nie mozna nagrywac podczas odtwarzania - pominieto");
        return *this;
    }
    recordList_ = ::LoadAutomationEventList(nullptr); // pusta lista o pełnej pojemności
    ::SetAutomationEventList(&recordList_);
    ::SetAutomationEventBaseFrame(0); // numery klatek nagrania liczone od zera
    ::StartAutomationEventRecording();
    recording_ = true;
    Log("Nagrywanie wejscia...");
    return *this;
}

bool Engine::StopRecording(const std::string& path) {
    if (!recording_) return false;
    ::StopAutomationEventRecording();
    const bool saved = ::ExportAutomationEventList(recordList_, path.c_str());
    Log(saved ? LogLevel::Info : LogLevel::Error, "Nagranie: " + std::to_string(recordList_.count) +
                                                      " zdarzen -> \"" + path + "\"" +
                                                      (saved ? "" : " - zapis nie powiodl sie"));
    ::UnloadAutomationEventList(recordList_);
    recordList_ = {};
    recording_ = false;
    return saved;
}

bool Engine::Replay(const std::string& path) {
    if (!RequireWindow("Replay")) return false;
    if (recording_) {
        Log(LogLevel::Warning, "nie mozna odtwarzac podczas nagrywania - pominieto");
        return false;
    }
    AutomationEventList list = ::LoadAutomationEventList(path.c_str());
    if (list.count == 0) {
        ::UnloadAutomationEventList(list);
        Log(LogLevel::Error, "brak zdarzen w nagraniu \"" + path + "\"");
        return false;
    }
    if (replaying_) ::UnloadAutomationEventList(replayList_);
    replayList_ = list;
    replayIndex_ = 0;
    replayFrame_ = 0;
    replaying_ = true;
    Log("Odtwarzanie nagrania \"" + path + "\" (" + std::to_string(list.count) + " zdarzen)");
    return true;
}
