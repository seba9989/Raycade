/**
 * @file EngineScaling.cpp
 * @brief Układ współrzędnych gry, skalowanie obrazu do okna i shader całego obrazu.
 */

#include <raycade/engine/Engine.hpp>

#include <rlgl.h>

#include <algorithm>
#include <cmath>

Engine& Engine::ViewSize(int width, int height) {
    viewWidth_ = width;
    viewHeight_ = height;
    if (initialized_ && scaling_ != Scaling::None) UpdateScaling();
    return *this;
}

Engine& Engine::ScaleMode(Scaling mode) {
    scaling_ = mode;
    if (!initialized_) return *this;
    if (mode == Scaling::None) {
        ReleaseTarget();
    } else {
        target_ = RenderTarget{}; // inny tryb to inny rozmiar tekstury
        UpdateScaling();          // nowy tryb działa od razu, a nie od następnej klatki
    }
    return *this;
}

Engine& Engine::AspectMode(Aspect mode) {
    aspect_ = mode;
    if (initialized_ && scaling_ != Scaling::None) UpdateScaling();
    return *this;
}

Engine& Engine::IntegerScaling(bool value) {
    integerScaling_ = value;
    if (initialized_ && scaling_ != Scaling::None) UpdateScaling();
    return *this;
}

Engine& Engine::PostShader(std::shared_ptr<ShaderProgram> shader) {
    postShader_ = std::move(shader);
    if (postShader_ && scaling_ == Scaling::None)
        Log(LogLevel::Warning, "PostShader() dziala tylko przy ScaleMode() innym niz None");
    return *this;
}

Engine& Engine::SmoothScaling(bool value) {
    smoothScaling_ = value;
    target_.Smooth(value);
    return *this;
}

Vector2 Engine::ViewSize() const {
    if (scaling_ != Scaling::None && viewWidth_ > 0)
        return {static_cast<float>(viewWidth_), static_cast<float>(viewHeight_)};
    return {static_cast<float>(ScreenWidth()), static_cast<float>(ScreenHeight())};
}

Vector2 Engine::VisibleSize() const {
    if (scaling_ != Scaling::None && visible_.x > 0.0f) return visible_;
    return ViewSize();
}

void Engine::ReleaseTarget() {
    target_ = RenderTarget{};
    visible_ = {0, 0};
    if (initialized_) input_.Transform({0, 0}, {1, 1});
}

/**
 * @details Skala (jednostki gry → piksele okna) zależy od AspectMode():
 * - Stretch: osobno dla każdej osi, okno / ViewSize,
 * - pozostałe: min(okno / ViewSize) w obu osiach.
 * Z IntegerScaling() skala jest zaokrąglana w dół, o ile wynosi co najmniej 1.
 *
 * Widoczny obszar to ViewSize, a przy KeepWidth / KeepHeight / Expand w danej osi
 * okno / skala (zaokrąglone w dół do całych jednostek). Obraz jest wyśrodkowany w oknie.
 *
 * Tekstura: przy Viewport ma rozmiar widocznego obszaru (1 jednostka = 1 piksel), przy Canvas -
 * rozmiar obrazu w oknie (1 piksel tekstury = 1 piksel okna). W obu przypadkach projekcja
 * ApplyViewProjection() mapuje widoczny obszar na całą teksturę.
 *
 * Mysz: raylib liczy (pozycja + przesunięcie) * skala, więc przesunięcie to minus lewy górny
 * róg obrazu, a skala to widoczny obszar / rozmiar obrazu w oknie.
 */
void Engine::UpdateScaling() {
    if (!initialized_ || scaling_ == Scaling::None) return;
    if (viewWidth_ <= 0 || viewHeight_ <= 0) {
        viewWidth_ = width_;
        viewHeight_ = height_;
    }
    const float screenWidth = static_cast<float>(::GetScreenWidth());
    const float screenHeight = static_cast<float>(::GetScreenHeight());
    // zminimalizowane okno - zostawiamy poprzednie ustawienia
    if (screenWidth <= 0.0f || screenHeight <= 0.0f) return;
    const float viewWidth = static_cast<float>(viewWidth_);
    const float viewHeight = static_cast<float>(viewHeight_);

    const auto snap = [this](float scale) {
        return integerScaling_ && scale >= 1.0f ? std::floor(scale) : scale;
    };
    float scaleX = screenWidth / viewWidth;
    float scaleY = screenHeight / viewHeight;
    if (aspect_ == Aspect::Stretch) {
        scaleX = snap(scaleX);
        scaleY = snap(scaleY);
    } else {
        scaleX = scaleY = snap(std::min(scaleX, scaleY));
    }

    const bool growX = aspect_ == Aspect::KeepHeight || aspect_ == Aspect::Expand;
    const bool growY = aspect_ == Aspect::KeepWidth || aspect_ == Aspect::Expand;
    visible_ = {
        growX ? std::max(viewWidth, std::floor(screenWidth / scaleX)) : viewWidth,
        growY ? std::max(viewHeight, std::floor(screenHeight / scaleY)) : viewHeight
    };

    const float width = visible_.x * scaleX;
    const float height = visible_.y * scaleY;
    destination_ = {
        std::floor((screenWidth - width) / 2), std::floor((screenHeight - height) / 2), width,
        height
    };

    const bool viewport = scaling_ == Scaling::Viewport;
    const int textureWidth =
        std::max(1, static_cast<int>(std::lround(viewport ? visible_.x : width)));
    const int textureHeight =
        std::max(1, static_cast<int>(std::lround(viewport ? visible_.y : height)));
    if (!target_.IsValid() || target_.Width() != textureWidth ||
        target_.Height() != textureHeight) {
        target_ = RenderTarget{textureWidth, textureHeight};
        target_.Smooth(smoothScaling_);
    }

    input_.Transform(
        {-destination_.x, -destination_.y},
        {visible_.x / destination_.width, visible_.y / destination_.height}
    );
}

/**
 * @details Macierz jak rlOrtho(0, w, h, 0, 0, 1) z BeginTextureMode(), tylko dla VisibleSize().
 * rlSetMatrixProjection() podmienia samą projekcję i nie zmienia trybu macierzy rlgl, więc nie
 * psuje przekształcenia odłożonego przez RenderTarget::Begin(); RenderTarget::End() przywróci
 * projekcję sprzed Begin().
 */
void Engine::ApplyViewProjection() const {
    const float width = visible_.x;
    const float height = visible_.y;
    Matrix projection{};
    projection.m0 = 2.0f / width;
    projection.m5 = -2.0f / height;
    projection.m10 = -2.0f;
    projection.m12 = -1.0f;
    projection.m13 = 1.0f;
    projection.m14 = -1.0f;
    projection.m15 = 1.0f;
    ::rlSetMatrixProjection(projection);
}
