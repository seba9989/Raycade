/**
 * @file EngineScaling.cpp
 * @brief Rozdzielczość gry, skalowanie obrazu do okna i shader całego obrazu.
 */

#include <raycade/Engine.hpp>

#include <algorithm>
#include <cmath>

Engine& Engine::Resolution(int width, int height) {
    viewWidth_ = width;
    viewHeight_ = height;
    if (initialized_ && scaling_ != Scaling::None) CreateTarget();
    return *this;
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
    target_ = RenderTarget{viewWidth_, viewHeight_};
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
 * - Fit: skala min(okno / gra) z zachowaniem proporcji, wyśrodkowana,
 * - PixelPerfect: jak Fit, ale skala zaokrąglona w dół do całkowitej (co najmniej 1).
 *
 * Mysz: raylib liczy (pozycja + przesunięcie) * skala, więc przesunięcie to minus lewy górny
 * róg obrazu, a skala to rozdzielczość gry / rozmiar obrazu w oknie.
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
        float scale = std::min(screenWidth / viewWidth, screenHeight / viewHeight);
        // całkowita skala tylko, gdy obraz mieści się w oknie co najmniej 1:1; w mniejszym
        // oknie zachowujemy się jak Fit, zamiast ucinać obraz
        if (scaling_ == Scaling::PixelPerfect && scale >= 1.0f) scale = std::floor(scale);
        const float width = viewWidth * scale;
        const float height = viewHeight * scale;
        destination_ = {std::floor((screenWidth - width) / 2),
                        std::floor((screenHeight - height) / 2), width, height};
    }
    input_.Transform({-destination_.x, -destination_.y},
                     {viewWidth / destination_.width, viewHeight / destination_.height});
}
