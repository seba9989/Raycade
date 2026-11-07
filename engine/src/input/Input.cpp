/**
 * @file Input.cpp
 * @brief Odczyt akcji i osi, przeliczanie myszy i dotyku na współrzędne gry.
 */
#include <raycade/input/Input.hpp>

Vector2 Input::MouseDelta() const {
    if (!mouse_) return {0, 0};
    // raylib skaluje pozycję kursora (SetMouseScale), ale nie jego przesunięcie
    const Vector2 delta = ::GetMouseDelta();
    return {delta.x * scale_.x, delta.y * scale_.y};
}

std::string Input::GamepadName(int gamepad) const {
    if (!GamepadAvailable(gamepad)) return {};
    const char* name = ::GetGamepadName(gamepad);
    return name ? name : "";
}

Vector2 Input::TouchPosition(int index) const {
    if (!touch_) return {0, 0};
    // raylib nie przelicza dotyku przez przesunięcie i skalę ekranu - robimy to tutaj
    const Vector2 position = ::GetTouchPosition(index);
    return {(position.x + offset_.x) * scale_.x, (position.y + offset_.y) * scale_.y};
}

Input& Input::BindAction(const std::string& action, std::vector<InputBinding> bindings) {
    actions_->actions[action] = std::move(bindings);
    actions_->analogDown.erase(action);
    return *this;
}

Input& Input::Unbind(const std::string& name) {
    actions_->actions.erase(name);
    actions_->axes.erase(name);
    actions_->analogDown.erase(name);
    return *this;
}

bool Input::ActionDown(const std::string& action) const {
    const auto* bindings = Find(actions_->actions, action);
    if (!bindings) return false;
    return std::any_of(bindings->begin(), bindings->end(),
                       [&](const InputBinding& b) { return BindingDown(b); });
}

bool Input::ActionPressed(const std::string& action) const {
    const auto* bindings = Find(actions_->actions, action);
    if (!bindings) return false;
    for (const InputBinding& b : *bindings)
        if (b.kind != InputBinding::Kind::PadAxis && BindingPressed(b)) return true;
    return AnalogDown(*bindings) && !WasAnalogDown(action);
}

bool Input::ActionReleased(const std::string& action) const {
    const auto* bindings = Find(actions_->actions, action);
    if (!bindings || ActionDown(action)) return false;
    for (const InputBinding& b : *bindings)
        if (b.kind != InputBinding::Kind::PadAxis && BindingReleased(b)) return true;
    return WasAnalogDown(action) && !AnalogDown(*bindings);
}

float Input::Axis(const std::string& axis) const {
    const auto* definition = Find(actions_->axes, axis);
    if (!definition) return 0.0f;
    float value = 0.0f;
    for (const InputBinding& b : definition->negative)
        if (BindingDown(b)) value -= 1.0f;
    for (const InputBinding& b : definition->positive)
        if (BindingDown(b)) value += 1.0f;
    for (const InputBinding& b : definition->analog) {
        const float v = GamepadAxis(b.gamepad, b.code) * b.direction;
        if (std::fabs(v) > definition->deadzone) value += v;
    }
    return std::clamp(value, -1.0f, 1.0f);
}

Vector2 Input::AxisVector(const std::string& x, const std::string& y) const {
    Vector2 v{Axis(x), Axis(y)};
    const float length = std::sqrt(v.x * v.x + v.y * v.y);
    if (length > 1.0f) v = {v.x / length, v.y / length};
    return v;
}

bool Input::BindingDown(const InputBinding& b) const {
    switch (b.kind) {
    case InputBinding::Kind::Key:
        return KeyDown(b.code);
    case InputBinding::Kind::Mouse:
        return MouseDown(b.code);
    case InputBinding::Kind::PadButton:
        return GamepadDown(b.gamepad, b.code);
    case InputBinding::Kind::PadAxis:
        return GamepadAxis(b.gamepad, b.code) * b.direction > b.threshold;
    }
    return false;
}

bool Input::BindingPressed(const InputBinding& b) const {
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

bool Input::BindingReleased(const InputBinding& b) const {
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

bool Input::AnalogDown(const std::vector<InputBinding>& bindings) const {
    return std::any_of(bindings.begin(), bindings.end(), [&](const InputBinding& b) {
        return b.kind == InputBinding::Kind::PadAxis && BindingDown(b);
    });
}

void Input::Transform(Vector2 offset, Vector2 scale) {
    offset_ = offset;
    scale_ = scale;
    ::SetMouseOffset(static_cast<int>(offset.x), static_cast<int>(offset.y));
    ::SetMouseScale(scale.x, scale.y);
}

Input Input::Uncaptured() const {
    Input copy = *this;
    copy.captured_ = false;
    return copy;
}

Input& Input::BindAxis(const std::string& axis, std::vector<InputBinding> negative,
                       std::vector<InputBinding> positive, std::vector<InputBinding> analog,
                       float deadzone) {
    actions_->axes[axis] =
        InputAxis{std::move(negative), std::move(positive), std::move(analog), deadzone};
    return *this;
}
