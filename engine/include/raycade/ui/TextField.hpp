/**
 * @file TextField.hpp
 * @brief Pole tekstowe z kursorem, edycją i wklejaniem.
 */
#pragma once

#include <raycade/entity/Entity.hpp>
#include <raycade/entity/Focusable.hpp>

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

/** @brief Elementy interfejsu użytkownika. */
namespace ui {

/**
 * @brief Jednowierszowe pole tekstowe.
 * @details Kliknięcie daje polu fokus i ustawia kursor; pole z fokusem przejmuje klawiaturę
 * (patrz Focusable), więc wpisywane litery nie sterują grą. Obsługuje:
 * - wpisywanie znaków (Unicode, przez Input::NextCharPressed()),
 * - strzałki w lewo/prawo, Home, End,
 * - Backspace i Delete (z powtarzaniem przy przytrzymaniu),
 * - Ctrl+V - wklejanie ze schowka,
 * - Enter - OnSubmit(), Escape - utrata fokusu,
 * - przewijanie tekstu dłuższego niż pole.
 *
 * Domyślna czcionka raylib nie ma polskich liter - wpisują się poprawnie (Text() zwraca UTF-8),
 * ale są wyświetlane jako "?".
 * @code{.cpp}
 * scene.Spawn<ui::TextField>()
 *     .Placeholder("Twoje imie")
 *     .MaxLength(20)
 *     .Position({100, 100})
 *     .OnSubmit([](ui::TextField& self, const std::string& text) { Save(text); self.Text(""); });
 * @endcode
 */
class TextField : public Entity, public Focusable {
  public:
    TextField() { Paint(DARKGRAY); }

    /** @name Ustawienia */
    /** @{ */

    /**
     * @brief Ustawia tekst (UTF-8) i przenosi kursor na koniec.
     * @details Nie wywołuje OnChange().
     */
    template <typename Self> Self& Text(this Self& self, const std::string& text) {
        self.codepoints_ = Decode(text);
        if (self.maxLength_ > 0 && self.codepoints_.size() > self.maxLength_)
            self.codepoints_.resize(self.maxLength_);
        self.caret_ = self.codepoints_.size();
        self.firstVisible_ = 0;
        return self;
    }

    /** @brief Zwraca tekst w UTF-8. */
    std::string Text() const { return Encode(codepoints_.begin(), codepoints_.end()); }

    /** @brief Ustawia napis widoczny w pustym polu. */
    template <typename Self> Self& Placeholder(this Self& self, std::string text) {
        self.placeholder_ = std::move(text);
        return self;
    }

    /** @brief Ustawia rozmiar pola (domyślnie 260 x 40). */
    template <typename Self> Self& Size(this Self& self, Vector2 size) {
        self.size_ = size;
        return self;
    }

    /** @brief Ustawia wielkość czcionki (domyślnie 20). */
    template <typename Self> Self& FontSize(this Self& self, int size) {
        self.fontSize_ = size;
        return self;
    }

    /** @brief Ogranicza liczbę znaków (0 - bez limitu). */
    template <typename Self> Self& MaxLength(this Self& self, std::size_t length) {
        self.maxLength_ = length;
        return self;
    }

    /** @} */

    /** @name Zdarzenia */
    /** @{ */

    /** @brief Reakcja na każdą zmianę tekstu przez użytkownika. */
    template <typename Self>
    Self& OnChange(this Self& self, Callback<Self, const std::string&> callback) {
        self.change_ = [&self, fn = std::move(callback)](const std::string& text) {
            fn(self, text);
        };
        return self;
    }

    /** @brief Reakcja na Enter. */
    template <typename Self>
    Self& OnSubmit(this Self& self, Callback<Self, const std::string&> callback) {
        self.submit_ = [&self, fn = std::move(callback)](const std::string& text) {
            fn(self, text);
        };
        return self;
    }

    /** @} */

    /** @brief Pozycja kursora (w znakach, nie bajtach). */
    std::size_t Caret() const { return caret_; }

    /** @brief Podaje napis wyświetlany w pustym polu. */
    const std::string& Placeholder() const { return placeholder_; }

    /** @brief Podaje rozmiar pola. */
    Vector2 Size() const { return size_; }

    /** @brief Podaje wielkość czcionki. */
    int FontSize() const { return fontSize_; }

    /** @brief Podaje limit znaków (0 - brak limitu). */
    std::size_t MaxLength() const { return maxLength_; }

    /** @brief Pole z fokusem przejmuje klawiaturę. */
    bool CapturesKeyboard() const override { return true; }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override {
        const auto local = TryToLocal(point);
        return local && ::CheckCollisionPointRec(*local, {0, 0, size_.x, size_.y});
    }

  protected:
    void Tick(float, const Input& input) override {
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
            if (change_) change_(Text());
        }
        if (input.KeyPressed(KEY_ENTER) || input.KeyPressed(KEY_KP_ENTER))
            if (submit_) submit_(Text());
        if (input.KeyPressed(KEY_ESCAPE)) Blur();
    }

    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(RAYWHITE).Fill().Rect({0, 0}, size_);
        renderer.Paint(IsFocused() ? SKYBLUE : (IsHovered() ? GRAY : LIGHTGRAY)).Fill(false);
        renderer.Rect({0, 0}, size_);
        if (IsFocused()) renderer.Rect({1, 1}, {size_.x - 2, size_.y - 2});

        const int textY = static_cast<int>((size_.y - static_cast<float>(fontSize_)) / 2);
        if (codepoints_.empty() && !IsFocused()) {
            ::DrawText(placeholder_.c_str(), kPadding, textY, fontSize_, GRAY);
            return;
        }
        KeepCaretVisible();
        const std::size_t end = VisibleEnd();
        const std::string visible =
            Encode(codepoints_.begin() + Offset(firstVisible_), codepoints_.begin() + Offset(end));
        ::DrawText(visible.c_str(), kPadding, textY, fontSize_, color_);

        // kursor miga co pół sekundy, ale zaraz po pisaniu jest zawsze widoczny
        const bool blinkOn = std::fmod(::GetTime() - blinkStart_, 1.0) < 0.5;
        if (IsFocused() && blinkOn) {
            const float x = static_cast<float>(kPadding + Width(firstVisible_, caret_));
            renderer.Paint(color_).Fill().Rect({x, static_cast<float>(textY)},
                                               {2, static_cast<float>(fontSize_)});
        }
    }

  private:
    static constexpr int kPadding = 8; /**< Odstęp tekstu od krawędzi. */

    static std::ptrdiff_t Offset(std::size_t index) { return static_cast<std::ptrdiff_t>(index); }

    /** @brief Zamienia UTF-8 na kody znaków Unicode. */
    static std::vector<int> Decode(const std::string& text) {
        std::vector<int> result;
        for (const char* p = text.c_str(); *p != '\0';) {
            int size = 0;
            result.push_back(::GetCodepointNext(p, &size));
            p += size > 0 ? size : 1;
        }
        return result;
    }

    /** @brief Zamienia fragment kodów znaków na UTF-8. */
    static std::string Encode(std::vector<int>::const_iterator begin,
                              std::vector<int>::const_iterator end) {
        std::string result;
        for (auto it = begin; it != end; ++it) {
            int size = 0;
            const char* bytes = ::CodepointToUTF8(*it, &size);
            result.append(bytes, static_cast<std::size_t>(size));
        }
        return result;
    }

    /** @brief Tekst ze schowka (pusty bez okna). */
    static std::string Clipboard() {
        if (!::IsWindowReady()) return {};
        const char* text = ::GetClipboardText();
        return text ? text : "";
    }

    /** @brief Wstawia znaki w miejscu kursora (z limitem długości). */
    bool Insert(const std::vector<int>& text) {
        std::vector<int> clean;
        for (int c : text)
            if (c >= 32) clean.push_back(c); // bez znaków sterujących (np. nowej linii)
        if (maxLength_ > 0)
            clean.resize(
                std::min(clean.size(), maxLength_ - std::min(maxLength_, codepoints_.size())));
        if (clean.empty()) return false;
        codepoints_.insert(codepoints_.begin() + Offset(caret_), clean.begin(), clean.end());
        caret_ += clean.size();
        return true;
    }

    /** @brief Szerokość fragmentu tekstu w pikselach. */
    int Width(std::size_t from, std::size_t to) const {
        if (to <= from) return 0;
        const std::string part =
            Encode(codepoints_.begin() + Offset(from), codepoints_.begin() + Offset(to));
        return ::MeasureText(part.c_str(), fontSize_);
    }

    /** @brief Szerokość obszaru na tekst. */
    int InnerWidth() const { return static_cast<int>(size_.x) - 2 * kPadding; }

    /** @brief Przewija tekst tak, żeby kursor był widoczny. */
    void KeepCaretVisible() const {
        if (caret_ < firstVisible_) firstVisible_ = caret_;
        while (firstVisible_ < caret_ && Width(firstVisible_, caret_) > InnerWidth())
            ++firstVisible_;
    }

    /** @brief Pierwszy znak, który już nie mieści się w polu. */
    std::size_t VisibleEnd() const {
        std::size_t end = firstVisible_;
        while (end < codepoints_.size() && Width(firstVisible_, end + 1) <= InnerWidth())
            ++end;
        return end;
    }

    /** @brief Ustawia kursor najbliżej miejsca kliknięcia. */
    void PlaceCaret(const Input& input) {
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

    std::vector<int> codepoints_;                    /**< Tekst jako kody znaków Unicode. */
    std::size_t caret_ = 0;                          /**< Pozycja kursora (w znakach). */
    mutable std::size_t firstVisible_ = 0;           /**< Pierwszy widoczny znak. */
    std::string placeholder_;                        /**< Napis w pustym polu. */
    Vector2 size_{260, 40};                          /**< Rozmiar pola. */
    int fontSize_ = 20;                              /**< Wielkość czcionki. */
    std::size_t maxLength_ = 0;                      /**< Limit znaków (0 - brak). */
    double blinkStart_ = 0.0;                        /**< Początek cyklu migania kursora. */
    std::function<void(const std::string&)> change_; /**< Zmiana tekstu. */
    std::function<void(const std::string&)> submit_; /**< Enter. */
};

} // namespace ui
