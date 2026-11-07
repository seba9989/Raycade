/**
 * @file ClipStack.cpp
 * @brief Przeliczanie prostokąta przez macierze rlgl na piksele bufora i stos scissorów.
 * @details Wierzchołki rlgl przechodzą przez trzy macierze: "transform" (rlPushMatrix() obiektów,
 * nakładana na CPU), widok (kamera, BeginMode2D()) i projekcję (ortho bieżącego bufora).
 * Rogi prostokąta przechodzą tę samą drogę do współrzędnych znormalizowanych (NDC), a stamtąd
 * do pikseli bufora w układzie OpenGL (początek w lewym dolnym rogu), czyli wprost do
 * rlScissor(). Dzięki temu wynik jest poprawny na ekranie, w teksturze, przy HighDPI
 * i przy skalowaniu obrazu gry.
 */

#include <raycade/render/ClipStack.hpp>

#include <rlgl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace {

/** @brief Prostokąt w pikselach bufora (układ OpenGL: y rośnie w górę). */
struct PixelRect {
    int x = 0;      /**< Lewa krawędź. */
    int y = 0;      /**< Dolna krawędź. */
    int width = 0;  /**< Szerokość (0 - nic nie widać). */
    int height = 0; /**< Wysokość. */
};

/** @brief Jeden bufor: ekran albo tekstura RenderTarget. */
struct Level {
    int width = 0;                /**< Szerokość bufora (0 - ekran, odczytywana na bieżąco). */
    int height = 0;               /**< Wysokość bufora. */
    std::vector<PixelRect> clips; /**< Przycięcia od najstarszego; ostatnie jest aktywne. */
};

/** @brief Poziomy od ekranu (pierwszy) do najbardziej zagnieżdżonej tekstury. */
std::vector<Level>& Levels() {
    static std::vector<Level> levels{Level{}};
    return levels;
}

/** @brief Rozmiar bufora poziomu; dla ekranu - bieżący rozmiar w pikselach. */
std::array<int, 2> BufferSize(const Level& level) {
    if (level.width > 0) return {level.width, level.height};
    return {::GetRenderWidth(), ::GetRenderHeight()};
}

/** @brief Mnoży macierz raylib (kolumnowo, jak w shaderze) przez punkt jednorodny. */
std::array<float, 4> Apply(const Matrix& m, const std::array<float, 4>& v) {
    return {
        m.m0 * v[0] + m.m4 * v[1] + m.m8 * v[2] + m.m12 * v[3],
        m.m1 * v[0] + m.m5 * v[1] + m.m9 * v[2] + m.m13 * v[3],
        m.m2 * v[0] + m.m6 * v[1] + m.m10 * v[2] + m.m14 * v[3],
        m.m3 * v[0] + m.m7 * v[1] + m.m11 * v[2] + m.m15 * v[3]
    };
}

/** @brief Włącza scissor z podanym prostokątem albo go wyłącza (nullptr). */
void Apply(const PixelRect* rect) {
    ::rlDrawRenderBatchActive(); // to, co już narysowano, zostaje z poprzednim przycięciem
    if (!rect) {
        ::rlDisableScissorTest();
        return;
    }
    ::rlEnableScissorTest();
    ::rlScissor(rect->x, rect->y, rect->width, rect->height);
}

/** @brief Włącza przycięcie bieżącego poziomu (ostatnie z jego listy) albo je wyłącza. */
void ApplyTop() {
    const Level& level = Levels().back();
    Apply(level.clips.empty() ? nullptr : &level.clips.back());
}

} // namespace

void ClipStack::Push(Rectangle area) {
    Level& level = Levels().back();
    const auto [width, height] = BufferSize(level);
    const Matrix transform = ::rlGetMatrixTransform();
    const Matrix view = ::rlGetMatrixModelview();
    const Matrix projection = ::rlGetMatrixProjection();

    float left = INFINITY, right = -INFINITY, bottom = INFINITY, top = -INFINITY;
    const std::array<std::array<float, 2>, 4> corners{
        {{area.x, area.y},
         {area.x + area.width, area.y},
         {area.x, area.y + area.height},
         {area.x + area.width, area.y + area.height}}
    };
    for (const auto& [x, y] : corners) {
        const auto clip = Apply(projection, Apply(view, Apply(transform, {x, y, 0.0f, 1.0f})));
        const float w = clip[3] != 0.0f ? clip[3] : 1.0f;
        const float px = (clip[0] / w + 1.0f) * 0.5f * static_cast<float>(width);
        const float py = (clip[1] / w + 1.0f) * 0.5f * static_cast<float>(height);
        left = std::min(left, px);
        right = std::max(right, px);
        bottom = std::min(bottom, py);
        top = std::max(top, py);
    }

    // zaokrąglenie na zewnątrz i przecięcie z poprzednim przycięciem (albo całym buforem)
    const PixelRect outer =
        level.clips.empty() ? PixelRect{0, 0, width, height} : level.clips.back();
    const int x0 = std::max(static_cast<int>(std::floor(left)), outer.x);
    const int y0 = std::max(static_cast<int>(std::floor(bottom)), outer.y);
    const int x1 = std::min(static_cast<int>(std::ceil(right)), outer.x + outer.width);
    const int y1 = std::min(static_cast<int>(std::ceil(top)), outer.y + outer.height);
    level.clips.push_back({x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)});
    ApplyTop();
}

void ClipStack::Pop() {
    Level& level = Levels().back();
    if (level.clips.empty()) return;
    level.clips.pop_back();
    ApplyTop();
}

bool ClipStack::IsActive() {
    return !Levels().back().clips.empty();
}

void ClipStack::BeginTarget(int width, int height) {
    // bufor poprzedniego celu został już opróżniony przez BeginTextureMode()
    Levels().push_back({width, height, {}});
    Apply(nullptr);
}

void ClipStack::EndTarget() {
    if (Levels().size() <= 1) return; // poziomu ekranu nie zamykamy
    Levels().pop_back();
    ApplyTop();
}
