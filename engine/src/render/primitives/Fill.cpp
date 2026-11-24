/**
 * @file Fill.cpp
 * @brief Iteracyjne algorytmy boundary fill i flood fill na bitmapie RAM oraz Blit()
 * (na ekran albo kopia pikseli do bitmapy).
 * @details Stos pikseli zastępuje rekurencję. Każdy krok bada czterech sąsiadów.
 */

#include <raycade/render/primitives/PrimitiveRenderer.hpp>

#include "Pixel.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

using pixel::Round;

/**
 * @details Wersja iteracyjna - rekurencję zastępuje stos, żeby duży obszar nie przepełnił
 * stosu wywołań:
 * 1. odłóż ziarno na stos,
 * 2. dopóki stos nie jest pusty: zdejmij piksel P; pomiń go, jeśli ma kolor wypełnienia
 *    albo kolor krawędzi; w przeciwnym razie pokoloruj P i odłóż jego sąsiadów N, S, W, E.
 */
PrimitiveRenderer& PrimitiveRenderer::BoundaryFill(Point2D seed, Color boundary) {
    if (!target_) return *this; // z ekranu nie da się odczytać kolorów

    std::vector<std::pair<int, int>> stack{{Round(seed.X()), Round(seed.Y())}};
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
PrimitiveRenderer& PrimitiveRenderer::FloodFill(Point2D seed) {
    if (!target_) return *this;

    const int seedX = Round(seed.X());
    const int seedY = Round(seed.Y());
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

PrimitiveRenderer& PrimitiveRenderer::Blit(const Bitmap& bitmap, Point2D position, Color tint) {
    const Rectangle whole{
        0.0f, 0.0f, static_cast<float>(bitmap.Width()), static_cast<float>(bitmap.Height())
    };
    return Blit(bitmap, whole, position, tint);
}

/**
 * @details Kopia do bitmapy:
 * 1. Odbicie (ujemny wymiar @p source) albo kopia na samą siebie: fragment trafia najpierw
 *    do osobnej bitmapy (Region()), która jest odbijana i kopiowana dalej jak zwykłe źródło -
 *    ImageDraw nie odbija, a przy wspólnej pamięci źródła i celu nadpisywałby czytane piksele.
 * 2. Fragment i pozycja są przycinane w liczbach całkowitych do granic źródła i celu, więc
 *    ImageDraw dostaje prostokąty o równych rozmiarach (bez skalowania), leżące w bitmapach.
 */
PrimitiveRenderer& PrimitiveRenderer::Blit(
    const Bitmap& bitmap,
    Rectangle source,
    Point2D position,
    Color tint
) {
    if (bitmap.IsEmpty()) return *this;
    if (!target_) {
        ::DrawTextureRec(bitmap.Texture(), source, position, tint);
        return *this;
    }

    const bool flipX = source.width < 0.0f;
    const bool flipY = source.height < 0.0f;
    source.width = std::fabs(source.width);
    source.height = std::fabs(source.height);

    // granice fragmentu tak jak w Bitmap::Region()
    const int left = static_cast<int>(std::floor(source.x));
    const int top = static_cast<int>(std::floor(source.y));
    const int width = static_cast<int>(std::floor(source.x + source.width)) - left;
    const int height = static_cast<int>(std::floor(source.y + source.height)) - top;

    if (flipX || flipY || &bitmap == target_) {
        Bitmap piece = bitmap.Region(source);
        if (piece.IsEmpty()) return *this;
        // Region() obcina fragment do źródła - przesunięcie zachowuje miejsce widocznej części
        int offsetX = std::max(0, -left);
        int offsetY = std::max(0, -top);
        if (flipX) {
            ::ImageFlipHorizontal(&piece.Edit());
            offsetX = width - offsetX - piece.Width();
        }
        if (flipY) {
            ::ImageFlipVertical(&piece.Edit());
            offsetY = height - offsetY - piece.Height();
        }
        const Point2D at{position.X() + offsetX, position.Y() + offsetY};
        return Blit(piece, at, tint);
    }

    int srcX = left;
    int srcY = top;
    int dstX = Round(position.X());
    int dstY = Round(position.Y());
    int w = width;
    int h = height;

    // przycięcie do źródła: brakujące piksele z lewej / z góry przesuwają też cel
    if (srcX < 0) {
        dstX -= srcX;
        w += srcX;
        srcX = 0;
    }
    if (srcY < 0) {
        dstY -= srcY;
        h += srcY;
        srcY = 0;
    }
    w = std::min(w, bitmap.Width() - srcX);
    h = std::min(h, bitmap.Height() - srcY);

    // przycięcie do celu
    if (dstX < 0) {
        srcX -= dstX;
        w += dstX;
        dstX = 0;
    }
    if (dstY < 0) {
        srcY -= dstY;
        h += dstY;
        dstY = 0;
    }
    w = std::min(w, target_->Width() - dstX);
    h = std::min(h, target_->Height() - dstY);
    if (w <= 0 || h <= 0) return *this;

    const auto fw = static_cast<float>(w);
    const auto fh = static_cast<float>(h);
    ::ImageDraw(
        &target_->Edit(), bitmap.image_,
        {static_cast<float>(srcX), static_cast<float>(srcY), fw, fh},
        {static_cast<float>(dstX), static_cast<float>(dstY), fw, fh}, tint
    );
    return *this;
}
