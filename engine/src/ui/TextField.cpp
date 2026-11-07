/**
 * @file TextField.cpp
 * @brief Edycja tekstu UTF-8, kursor, schowek i przewijanie pola tekstowego.
 */
#include <raycade/ui/TextField.hpp>

#include <cmath>

namespace ui {

bool TextField::Contains(Vector2 point) const {
    const auto local = TryToLocal(point);
    return local && ::CheckCollisionPointRec(*local, {0, 0, size_.x, size_.y});
}

void TextField::Tick(float, const Input& input) {
    if (!IsFocused()) return;
    if (IsHovered() && input.MousePressed(MOUSE_BUTTON_LEFT)) PlaceCaret(input);

    bool changed = false;
    for (int c = input.NextCharPressed(); c != 0; c = input.NextCharPressed())
        if (c >= 32) changed |= Insert({c});

    const auto pressed = [&](int key) {
        return input.KeyPressed(key) || input.KeyPressedRepeat(key);
    };
    const bool ctrl = input.KeyDown(KEY_LEFT_CONTROL) || input.KeyDown(KEY_RIGHT_CONTROL);
    if (ctrl && input.KeyPressed(KEY_V)) changed |= Insert(Decode(Clipboard()));
    if (pressed(KEY_BACKSPACE) && caret_ > 0) {
        codepoints_.erase(codepoints_.begin() + static_cast<std::ptrdiff_t>(--caret_));
        changed = true;
    }
    if (pressed(KEY_DELETE) && caret_ < codepoints_.size()) {
        codepoints_.erase(codepoints_.begin() + static_cast<std::ptrdiff_t>(caret_));
        changed = true;
    }
    if (pressed(KEY_LEFT) && caret_ > 0) --caret_;
    if (pressed(KEY_RIGHT) && caret_ < codepoints_.size()) ++caret_;
    if (input.KeyPressed(KEY_HOME)) caret_ = 0;
    if (input.KeyPressed(KEY_END)) caret_ = codepoints_.size();

    if (changed) {
        blinkStart_ = ::GetTime();
        if (change_) change_(Content());
    }
    if (input.KeyPressed(KEY_ENTER) || input.KeyPressed(KEY_KP_ENTER))
        if (submit_) submit_(Content());
    if (input.KeyPressed(KEY_ESCAPE)) Blur();
}

void TextField::Draw(PrimitiveRenderer& renderer) const {
    renderer.Paint(RAYWHITE).Fill().Rect({0, 0}, size_);
    renderer.Paint(IsFocused() ? SKYBLUE : (IsHovered() ? GRAY : LIGHTGRAY)).Fill(false);
    renderer.Rect({0, 0}, size_);
    if (IsFocused()) renderer.Rect({1, 1}, {size_.x - 2, size_.y - 2});

    const float textY = std::floor((size_.y - fontSize_) / 2);
    const Vector2 textPosition{static_cast<float>(kPadding), textY};
    const Typeface& face = Face();
    if (codepoints_.empty() && !IsFocused()) {
        face.Draw(placeholder_, textPosition, fontSize_, Spacing(), GRAY);
        return;
    }
    KeepCaretVisible();
    const std::size_t end = VisibleEnd();
    const std::string visible =
        Encode(codepoints_.begin() + Offset(firstVisible_), codepoints_.begin() + Offset(end));
    face.Draw(visible, textPosition, fontSize_, Spacing(), color_);

    // kursor miga co pół sekundy, ale zaraz po pisaniu jest zawsze widoczny
    const bool blinkOn = std::fmod(::GetTime() - blinkStart_, 1.0) < 0.5;
    if (IsFocused() && blinkOn) {
        const float x = static_cast<float>(kPadding + Width(firstVisible_, caret_));
        renderer.Paint(color_).Fill().Rect({x, textY}, {2, fontSize_});
    }
}

std::vector<int> TextField::Decode(const std::string& text) {
    std::vector<int> result;
    for (const char* p = text.c_str(); *p != '\0';) {
        int size = 0;
        result.push_back(::GetCodepointNext(p, &size));
        p += size > 0 ? size : 1;
    }
    return result;
}

std::string TextField::Clipboard() {
    if (!::IsWindowReady()) return {};
    const char* text = ::GetClipboardText();
    return text ? text : "";
}

bool TextField::Insert(const std::vector<int>& text) {
    std::vector<int> clean;
    for (int c : text)
        if (c >= 32) clean.push_back(c); // bez znaków sterujących (np. nowej linii)
    if (maxLength_ > 0)
        clean.resize(std::min(clean.size(), maxLength_ - std::min(maxLength_, codepoints_.size())));
    if (clean.empty()) return false;
    codepoints_.insert(codepoints_.begin() + Offset(caret_), clean.begin(), clean.end());
    caret_ += clean.size();
    return true;
}

int TextField::Width(std::size_t from, std::size_t to) const {
    if (to <= from) return 0;
    const std::string part =
        Encode(codepoints_.begin() + Offset(from), codepoints_.begin() + Offset(to));
    return static_cast<int>(std::lround(Face().Measure(part, fontSize_, Spacing()).x));
}

void TextField::KeepCaretVisible() const {
    if (caret_ < firstVisible_) firstVisible_ = caret_;
    while (firstVisible_ < caret_ && Width(firstVisible_, caret_) > InnerWidth())
        ++firstVisible_;
}

std::size_t TextField::VisibleEnd() const {
    std::size_t end = firstVisible_;
    while (end < codepoints_.size() && Width(firstVisible_, end + 1) <= InnerWidth())
        ++end;
    return end;
}

void TextField::PlaceCaret(const Input& input) {
    const float x = ScreenToLocal(input.MousePosition()).x - kPadding;
    std::size_t best = firstVisible_;
    float bestDistance = std::fabs(x);
    for (std::size_t i = firstVisible_ + 1; i <= codepoints_.size(); ++i) {
        const float distance = std::fabs(x - static_cast<float>(Width(firstVisible_, i)));
        if (distance < bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }
    caret_ = best;
    blinkStart_ = ::GetTime();
}

std::string TextField::Encode(
    std::vector<int>::const_iterator begin,
    std::vector<int>::const_iterator end
) {
    std::string result;
    for (auto it = begin; it != end; ++it) {
        int size = 0;
        const char* bytes = ::CodepointToUTF8(*it, &size);
        result.append(bytes, static_cast<std::size_t>(size));
    }
    return result;
}

} // namespace ui
