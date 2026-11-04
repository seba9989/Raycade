/**
 * @file Fill.cpp
 * @brief Iteracyjne algorytmy boundary fill i flood fill na bitmapie RAM oraz Blit().
 * @details Stos pikseli zastępuje rekurencję. Każdy krok bada czterech sąsiadów.
 */

#include "../PrimitiveRenderer.hpp"

#include <cmath>
#include <utility>
#include <vector>

namespace {

/** @brief Zaokrągla współrzędną do najbliższego piksela. */
int Round(float value) {
    return static_cast<int>(std::lround(value));
}

} // namespace

/**
 * @details Wersja iteracyjna - rekurencję zastępuje stos, żeby duży obszar nie przepełnił
 * stosu wywołań:
 * 1. odłóż ziarno na stos,
 * 2. dopóki stos nie jest pusty: zdejmij piksel P; pomiń go, jeśli ma kolor wypełnienia
 *    albo kolor krawędzi; w przeciwnym razie pokoloruj P i odłóż jego sąsiadów N, S, W, E.
 */
PrimitiveRenderer& PrimitiveRenderer::BoundaryFill(Vector2 seed, Color boundary) {
    if (!target_) return *this; // z ekranu nie da się odczytać kolorów

    std::vector<std::pair<int, int>> stack{{Round(seed.x), Round(seed.y)}};
    while (!stack.empty()) {
        const auto [x, y] = stack.back();
        stack.pop_back();
        if (!target_->Contains(x, y)) continue;

        const Color pixel = target_->Pixel(x, y);
        if (::ColorIsEqual(pixel, color_) || ::ColorIsEqual(pixel, boundary)) continue;

        target_->Pixel(x, y, color_);
        stack.insert(stack.end(), {{x, y - 1}, {x, y + 1}, {x - 1, y}, {x + 1, y}});
    }
    return *this;
}

/**
 * @details Przebieg jak w BoundaryFill(), ale piksel jest pomijany, gdy ma kolor wypełnienia
 * albo kolor inny niż kolor ziarna (tła).
 */
PrimitiveRenderer& PrimitiveRenderer::FloodFill(Vector2 seed) {
    if (!target_) return *this;

    const int seedX = Round(seed.x);
    const int seedY = Round(seed.y);
    if (!target_->Contains(seedX, seedY)) return *this;
    const Color background = target_->Pixel(seedX, seedY);

    std::vector<std::pair<int, int>> stack{{seedX, seedY}};
    while (!stack.empty()) {
        const auto [x, y] = stack.back();
        stack.pop_back();
        if (!target_->Contains(x, y)) continue;

        const Color pixel = target_->Pixel(x, y);
        if (::ColorIsEqual(pixel, color_) || !::ColorIsEqual(pixel, background)) continue;

        target_->Pixel(x, y, color_);
        stack.insert(stack.end(), {{x, y - 1}, {x, y + 1}, {x - 1, y}, {x + 1, y}});
    }
    return *this;
}

PrimitiveRenderer& PrimitiveRenderer::Blit(const Bitmap& bitmap, Vector2 position) {
    ::DrawTextureV(bitmap.Texture(), position, WHITE);
    return *this;
}
