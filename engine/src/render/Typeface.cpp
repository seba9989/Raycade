/**
 * @file Typeface.cpp
 * @brief Wczytywanie czcionki z polskimi znakami, pomiar i rysowanie tekstu.
 */

#include <raycade/render/Typeface.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

std::optional<Typeface> Typeface::Load(
    const std::string& path,
    int size,
    const std::string& extraCharacters,
    bool smooth
) {
    // raylib wczytuje tylko podane znaki - bez listy byłoby samo ASCII 32-126
    std::string characters;
    for (char c = 32; c < 127; ++c)
        characters += c;
    characters += kPolishLetters;
    characters += extraCharacters;

    int count = 0;
    int* codepoints = ::LoadCodepoints(characters.c_str(), &count);
    const ::Font font = ::LoadFontEx(path.c_str(), size, codepoints, count);
    ::UnloadCodepoints(codepoints);
    if (!::IsFontValid(font)) return std::nullopt;

    // gładkie krawędzie glifów przy skalowaniu tekstu albo ostre piksele (font pikselowy)
    ::SetTextureFilter(font.texture, smooth ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
    Typeface result;
    result.font_ = font;
    result.loaded_ = true;
    return result;
}

const Typeface& Typeface::Default() {
    static const Typeface typeface;
    return typeface;
}

Typeface::~Typeface() {
    Release();
}

Typeface::Typeface(Typeface&& other) noexcept
    : font_(std::exchange(other.font_, {})), loaded_(std::exchange(other.loaded_, false)) {
}

Typeface& Typeface::operator=(Typeface&& other) noexcept {
    if (this != &other) {
        Release();
        font_ = std::exchange(other.font_, {});
        loaded_ = std::exchange(other.loaded_, false);
    }
    return *this;
}

void Typeface::Release() {
    // tekstura glifów wymaga żywego kontekstu OpenGL (przed CloseWindow)
    if (loaded_ && ::IsWindowReady()) ::UnloadFont(font_);
    font_ = {};
    loaded_ = false;
}

const ::Font& Typeface::Raw() const {
    if (loaded_) return font_;
    // font domyślny raylib powstaje przy InitWindow(), więc pobieramy go przy użyciu
    static ::Font fallback{};
    fallback = ::GetFontDefault();
    return fallback;
}

float Typeface::DefaultSpacing(float size) const {
    if (loaded_) return 0.0f; // TTF/OTF: odstępy są już w glifach
    return std::max(1.0f, std::floor(size / 10.0f));
}

Vector2 Typeface::Measure(const std::string& text, float size, float spacing) const {
    if (text.empty()) return {0, size};
    return ::MeasureTextEx(Raw(), text.c_str(), size, spacing);
}

void Typeface::Draw(
    const std::string& text,
    Vector2 position,
    float size,
    float spacing,
    Color color
) const {
    if (text.empty()) return;
    ::DrawTextEx(Raw(), text.c_str(), position, size, spacing, color);
}
