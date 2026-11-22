/**
 * @file EngineScaling.cpp
 * @brief Rozdzielczość gry, skalowanie obrazu do okna i shader całego obrazu.
 */

#include <raycade/engine/Engine.hpp>

#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <string>

Engine& Engine::ViewSize(int width, int height) {
    viewWidth_ = width;
    viewHeight_ = height;
    // tekstura zależy od ViewSize() tylko bez własnej Resolution(); inaczej wystarczy przeliczyć
    // obraz w oknie i mysz
    if (initialized_ && scaling_ != Scaling::None) {
        if (resolutionWidth_ > 0 && target_.IsValid())
            UpdateScaling();
        else
            CreateTarget();
    }
    return *this;
}

Engine& Engine::Resolution(int width, int height) {
    resolutionWidth_ = width;
    resolutionHeight_ = height;
    if (initialized_ && scaling_ != Scaling::None) CreateTarget();
    return *this;
}

Vector2 Engine::Resolution() const {
    if (target_.IsValid())
        return {static_cast<float>(target_.Width()), static_cast<float>(target_.Height())};
    if (scaling_ != Scaling::None && resolutionWidth_ > 0)
        return {static_cast<float>(resolutionWidth_), static_cast<float>(resolutionHeight_)};
    return ViewSize();
}

Engine& Engine::ScaleMode(Scaling mode) {
    scaling_ = mode;
    if (initialized_) {
        if (mode == Scaling::None)
            ReleaseTarget();
        else if (!target_.IsValid())
            CreateTarget();
        else
            UpdateScaling(); // nowy tryb działa od razu, a nie od następnej klatki
    }
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

void Engine::CreateTarget() {
    ReleaseTarget();
    if (viewWidth_ <= 0 || viewHeight_ <= 0) {
        viewWidth_ = width_;
        viewHeight_ = height_;
    }
    const bool custom = resolutionWidth_ > 0 && resolutionHeight_ > 0;
    const int width = custom ? resolutionWidth_ : viewWidth_;
    const int height = custom ? resolutionHeight_ : viewHeight_;
    // różne proporcje dają niekwadratowe piksele - obraz i tak wypełni ViewSize()
    const float viewAspect = static_cast<float>(viewWidth_) / static_cast<float>(viewHeight_);
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    if (std::abs(aspect - viewAspect) > 0.01f * viewAspect)
        Log(LogLevel::Warning,
            "Resolution() " + std::to_string(width) + "x" + std::to_string(height) +
                " ma inne proporcje niz ViewSize() " + std::to_string(viewWidth_) + "x" +
                std::to_string(viewHeight_) + " - piksele nie beda kwadratowe");
    target_ = RenderTarget{width, height};
    target_.Smooth(smoothScaling_);
    UpdateScaling();
}

void Engine::ReleaseTarget() {
    target_ = RenderTarget{};
    if (initialized_) input_.Transform({0, 0}, {1, 1});
}

/**
 * @details Prostokąt docelowy zależy od trybu:
 * - Stretch: całe okno,
 * - Fit: skala min(okno / ViewSize) z zachowaniem proporcji, wyśrodkowana,
 * - PixelPerfect: jak Fit, ale skala pikseli tekstury (okno / Resolution) zaokrąglona w dół
 *   do całkowitej (co najmniej 1) - ostre są piksele tekstury, a nie jednostki gry.
 *
 * Mysz: raylib liczy (pozycja + przesunięcie) * skala, więc przesunięcie to minus lewy górny
 * róg obrazu, a skala to rozmiar ViewSize() / rozmiar obrazu w oknie - mysz jest w jednostkach
 * gry niezależnie od Resolution().
 */
void Engine::UpdateScaling() {
    if (!target_.IsValid()) return;
    const float screenWidth = static_cast<float>(::GetScreenWidth());
    const float screenHeight = static_cast<float>(::GetScreenHeight());
    const float viewWidth = static_cast<float>(viewWidth_);
    const float viewHeight = static_cast<float>(viewHeight_);

    if (scaling_ == Scaling::Stretch) {
        destination_ = {0, 0, screenWidth, screenHeight};
    } else {
        float width = 0.0f;
        float height = 0.0f;
        const float textureWidth = static_cast<float>(target_.Width());
        const float textureHeight = static_cast<float>(target_.Height());
        const float pixelScale =
            std::min(screenWidth / textureWidth, screenHeight / textureHeight);
        // całkowita skala tylko, gdy tekstura mieści się w oknie co najmniej 1:1; w mniejszym
        // oknie zachowujemy się jak Fit, zamiast ucinać obraz
        if (scaling_ == Scaling::PixelPerfect && pixelScale >= 1.0f) {
            width = textureWidth * std::floor(pixelScale);
            height = textureHeight * std::floor(pixelScale);
        } else {
            const float scale = std::min(screenWidth / viewWidth, screenHeight / viewHeight);
            width = viewWidth * scale;
            height = viewHeight * scale;
        }
        destination_ = {
            std::floor((screenWidth - width) / 2), std::floor((screenHeight - height) / 2), width,
            height
        };
    }
    input_.Transform(
        {-destination_.x, -destination_.y},
        {viewWidth / destination_.width, viewHeight / destination_.height}
    );
}

/**
 * @details Macierz jak rlOrtho(0, w, h, 0, 0, 1) z BeginTextureMode(), tylko dla ViewSize().
 * rlSetMatrixProjection() podmienia samą projekcję i nie zmienia trybu macierzy rlgl, więc nie
 * psuje przekształcenia odłożonego przez RenderTarget::Begin(); RenderTarget::End() przywróci
 * projekcję sprzed Begin().
 */
void Engine::ApplyViewProjection() const {
    const float width = static_cast<float>(viewWidth_);
    const float height = static_cast<float>(viewHeight_);
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
