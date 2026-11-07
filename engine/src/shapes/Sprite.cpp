/**
 * @file Sprite.cpp
 * @brief Animacja klatkowa: arkusz, postęp czasu, wybór klatki i rysowanie.
 */
#include <raycade/shapes/Sprite.hpp>

#include <algorithm>

namespace shapes {

const SpriteClip& Sprite::ActiveClip() const {
    if (clip_) return *clip_;
    // animacja domyślna: cały arkusz z Fps() (pola aktualne przy każdym użyciu)
    defaultClip_.first = 0;
    defaultClip_.count = std::max(1, SheetFrames());
    defaultClip_.fps = fps_;
    return defaultClip_;
}

void Sprite::SetGrid(
    std::shared_ptr<const Bitmap> bitmap,
    Vector2 frame,
    Vector2 margin,
    Vector2 spacing
) {
    auto sheet =
        std::make_shared<SpriteSheet>(SpriteSheet::Grid(std::move(bitmap), frame, margin, spacing));
    // animacje z Animation() wywołanego przed Sheet() nie mogą zniknąć
    if (own_)
        for (const auto& [name, clip] : own_->Clips())
            sheet->Clip(name, clip);
    own_ = sheet;
    sheet_ = std::move(sheet);
    SheetChanged();
}

SpriteSheet& Sprite::MutableSheet() {
    if (!own_) {
        // kopia opisu (bitmapa zostaje wspólna) - zmiana nie dotyka innych sprite'ów
        own_ = sheet_ ? std::make_shared<SpriteSheet>(*sheet_) : std::make_shared<SpriteSheet>();
        sheet_ = own_;
    }
    return *own_;
}

void Sprite::SheetChanged() {
    ClipsChanged();
    Rewind(frame_);
}

void Sprite::ClipsChanged() {
    clip_ = sheet_ && !current_.empty() ? sheet_->FindClip(current_) : nullptr;
    ++generation_;
}

void Sprite::Start(const std::string& name) {
    const SpriteClip* clip = name.empty() || !sheet_ ? nullptr : sheet_->FindClip(name);
    if (!name.empty() && !clip) {
        if (name != missing_) {
            missing_ = name;
            ::TraceLog(LOG_WARNING, "Sprite: nieznana animacja \"%s\"", name.c_str());
        }
        return;
    }
    if (name == current_) {
        if (!playing_) Resume();
        return;
    }
    current_ = name;
    clip_ = clip;
    ++generation_;
    direction_ = 1;
    Rewind(0);
    playing_ = true;
}

void Sprite::Resume() {
    const SpriteClip& clip = ActiveClip();
    // jednorazowa animacja zatrzymana na końcu zaczyna od nowa
    if (clip.mode == Playback::Once && frame_ >= clip.Length() - 1) Rewind(0);
    playing_ = true;
}

void Sprite::Rewind(int frame) {
    frame_ = std::clamp(frame, 0, ActiveClip().Length() - 1);
    time_ = 0.0f;
}

int Sprite::SheetFrame() const {
    return std::clamp(ActiveClip().SheetFrame(frame_), 0, std::max(0, SheetFrames() - 1));
}

Vector2 Sprite::FrameSize() const {
    if (!IsLoaded()) return {0, 0};
    const Rectangle frame = sheet_->Frame(SheetFrame());
    return {frame.width, frame.height};
}

bool Sprite::Advance(const SpriteClip& clip) {
    const int length = clip.Length();
    switch (clip.mode) {
    case Playback::Loop:
        frame_ = (frame_ + 1) % length;
        return true;
    case Playback::PingPong:
        // odbicie na końcach: 0 1 2 1 0 1 ... (bez powtarzania skrajnej klatki)
        if (frame_ + direction_ < 0 || frame_ + direction_ >= length) direction_ = -direction_;
        frame_ += direction_;
        return true;
    case Playback::Once:
        if (frame_ + 1 < length) {
            ++frame_;
            return true;
        }
        return false;
    }
    return true;
}

void Sprite::Tick(float dt, const Input&) {
    if (!playing_ || speed_ <= 0.0f) return;
    const SpriteClip& clip = ActiveClip();
    if (clip.Length() <= 1) return;

    // pętla: przy długiej klatce gry animacja przeskakuje kilka klatek
    time_ += dt * speed_;
    const std::uint32_t generation = generation_;
    while (true) {
        const float duration = clip.Duration(frame_);
        if (duration <= 0.0f || time_ < duration) return;
        time_ -= duration;
        if (!Advance(clip)) {
            playing_ = false;
            time_ = 0.0f;
            // kopia nazwy: callback może zmienić animację (Play()) albo usunąć obiekt
            const std::string finished = current_;
            if (finish_) finish_(finished);
            return;
        }
        if (frameEvent_)
            for (const auto& [frame, event] : clip.events)
                if (frame == frame_) {
                    const std::string name = event; // jak wyżej - callback może zmienić arkusz
                    frameEvent_(name);
                    // callback zmienił animację albo arkusz - clip może już nie istnieć
                    if (generation != generation_ || !playing_) return;
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
    Rectangle source = sheet_->Frame(SheetFrame());
    // ujemny wymiar źródła odbija obraz (DrawTextureRec)
    if (flipX_) source.width = -source.width;
    if (flipY_) source.height = -source.height;
    renderer.Blit(*sheet_->Bitmap(), source, {-origin_.x, -origin_.y}, color_);
}

} // namespace shapes
