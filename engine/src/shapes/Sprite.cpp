/**
 * @file Sprite.cpp
 * @brief Animacja klatkowa: postęp czasu, wybór klatki z arkusza i rysowanie.
 */
#include <raycade/shapes/Sprite.hpp>

#include <algorithm>

namespace shapes {

Vector2 Sprite::FrameSize() const {
    if (!IsLoaded()) return {0, 0};
    const Vector2 sheet{static_cast<float>(sheet_->Width()), static_cast<float>(sheet_->Height())};
    if (frameSize_.x <= 0 || frameSize_.y <= 0) return sheet;
    return {std::min(frameSize_.x, sheet.x), std::min(frameSize_.y, sheet.y)};
}

int Sprite::SheetFrames() const {
    if (!IsLoaded()) return 0;
    const Vector2 frame = FrameSize();
    const int columns = static_cast<int>(static_cast<float>(sheet_->Width()) / frame.x);
    const int rows = static_cast<int>(static_cast<float>(sheet_->Height()) / frame.y);
    return std::max(1, columns) * std::max(1, rows);
}

Sprite::AnimationClip Sprite::Current() const {
    if (!current_.empty())
        if (const auto it = animations_.find(current_); it != animations_.end()) return it->second;
    // animacja domyślna: cały arkusz
    return {0, std::max(1, SheetFrames()), fps_, true};
}

void Sprite::Start(const std::string& name) {
    if (!name.empty() && !animations_.contains(name)) return;
    if (name == current_) {
        if (!playing_) Resume();
        return;
    }
    current_ = name;
    frame_ = 0;
    time_ = 0.0f;
    playing_ = true;
}

void Sprite::Resume() {
    const AnimationClip clip = Current();
    // jednorazowa animacja zatrzymana na końcu zaczyna od nowa
    if (!clip.loop && frame_ >= clip.count - 1) {
        frame_ = 0;
        time_ = 0.0f;
    }
    playing_ = true;
}

void Sprite::ClampFrame() {
    frame_ = std::clamp(frame_, 0, std::max(0, Current().count - 1));
}

int Sprite::SheetFrame() const {
    const AnimationClip clip = Current();
    return std::clamp(clip.first + frame_, 0, std::max(0, SheetFrames() - 1));
}

void Sprite::Tick(float dt, const Input&) {
    if (!playing_) return;
    const AnimationClip clip = Current();
    const float fps = clip.fps * speed_;
    if (fps <= 0.0f || clip.count <= 1) return;

    // pętla: przy długiej klatce (dt > 1 / fps) animacja przeskakuje kilka klatek
    time_ += dt;
    const float duration = 1.0f / fps;
    while (time_ >= duration) {
        time_ -= duration;
        if (frame_ + 1 < clip.count) {
            ++frame_;
        } else if (clip.loop) {
            frame_ = 0;
        } else {
            playing_ = false;
            time_ = 0.0f;
            // kopia nazwy: callback może zmienić animację (Play()) albo usunąć obiekt
            const std::string finished = current_;
            if (finish_) finish_(finished);
            return;
        }
    }
}

bool Sprite::Contains(Vector2 point) const {
    const auto local = TryToLocal(point);
    const Vector2 frame = FrameSize();
    return local && ::CheckCollisionPointRec(*local, {-origin_.x, -origin_.y, frame.x, frame.y});
}

void Sprite::Draw(PrimitiveRenderer& renderer) const {
    if (!IsLoaded()) return;
    const Vector2 frame = FrameSize();
    const int columns =
        std::max(1, static_cast<int>(static_cast<float>(sheet_->Width()) / frame.x));
    const int index = SheetFrame();
    const Rectangle source{static_cast<float>(index % columns) * frame.x,
                           static_cast<float>(index / columns) * frame.y,
                           flipX_ ? -frame.x : frame.x, flipY_ ? -frame.y : frame.y};
    renderer.Blit(*sheet_, source, {-origin_.x, -origin_.y}, color_);
}

} // namespace shapes
