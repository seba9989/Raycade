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
    bool Contains(Vector2 point) const override;

  protected:
    void Tick(float, const Input& input) override;

    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    static constexpr int kPadding = 8; /**< Odstęp tekstu od krawędzi. */

    static std::ptrdiff_t Offset(std::size_t index) { return static_cast<std::ptrdiff_t>(index); }

    /** @brief Zamienia UTF-8 na kody znaków Unicode. */
    static std::vector<int> Decode(const std::string& text);

    /** @brief Zamienia fragment kodów znaków na UTF-8. */
    static std::string Encode(std::vector<int>::const_iterator begin,
                              std::vector<int>::const_iterator end);

    /** @brief Tekst ze schowka (pusty bez okna). */
    static std::string Clipboard();

    /** @brief Wstawia znaki w miejscu kursora (z limitem długości). */
    bool Insert(const std::vector<int>& text);

    /** @brief Szerokość fragmentu tekstu w pikselach. */
    int Width(std::size_t from, std::size_t to) const;

    /** @brief Szerokość obszaru na tekst. */
    int InnerWidth() const { return static_cast<int>(size_.x) - 2 * kPadding; }

    /** @brief Przewija tekst tak, żeby kursor był widoczny. */
    void KeepCaretVisible() const;

    /** @brief Pierwszy znak, który już nie mieści się w polu. */
    std::size_t VisibleEnd() const;

    /** @brief Ustawia kursor najbliżej miejsca kliknięcia. */
    void PlaceCaret(const Input& input);

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
