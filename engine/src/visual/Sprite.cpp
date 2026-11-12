/**
 * @file Sprite.cpp
 * @brief Animacja klatkowa: arkusz, postęp czasu, wybór klatki i rysowanie.
 */
#include <raycade/visual/Sprite.hpp>

#include <algorithm>
#include <cctype>

namespace visual {

bool Sprite::HasFrames() const {
    return sheet_ && sheet_->Bitmap() && !sheet_->Bitmap()->IsEmpty() && sheet_->FrameCount() > 0;
}

bool Sprite::IsJsonPath(const std::string& path) {
    if (path.size() < 5) return false;
    std::string extension = path.substr(path.size() - 5);
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return extension == ".json";
}

const SpriteAnimation& Sprite::ActiveAnimation() const {
    if (clip_) return *clip_;
    // animacja domyślna: cały arkusz z Fps() (pola aktualne przy każdym użyciu)
    defaultClip_.first = 0;
    defaultClip_.count = std::max(1, SheetFrameCount());
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
        for (const auto& [name, clip] : own_->Animations())
            sheet->Animation(name, clip);
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
    // animacja domyślna bierze czasy klatek z pliku (np. Aseprite bez tagów), jeśli są
    defaultClip_.durations.clear();
    if (sheet_) {
        bool any = false;
        for (int i = 0; i < sheet_->FrameCount(); ++i)
            any |= sheet_->FrameInfo(i).duration > 0.0f;
        if (any)
            for (int i = 0; i < sheet_->FrameCount(); ++i)
                defaultClip_.durations.push_back(sheet_->FrameInfo(i).duration);
    }
    AnimationsChanged();
    Rewind(frame_);
}

void Sprite::AnimationsChanged() {
    clip_ = sheet_ && !current_.empty() ? sheet_->FindAnimation(current_) : nullptr;
    ++generation_;
}

void Sprite::Start(const std::string& name) {
    const SpriteAnimation* clip = name.empty() || !sheet_ ? nullptr : sheet_->FindAnimation(name);
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
    const SpriteAnimation& clip = ActiveAnimation();
    // jednorazowa animacja zatrzymana na końcu zaczyna od nowa
    if (clip.mode == Playback::Once && frame_ >= clip.Length() - 1) Rewind(0);
    playing_ = true;
}

void Sprite::Rewind(int frame) {
    frame_ = std::clamp(frame, 0, ActiveAnimation().Length() - 1);
    time_ = 0.0f;
}

int Sprite::SheetFrame() const {
    return std::clamp(ActiveAnimation().SheetFrame(frame_), 0, std::max(0, SheetFrameCount() - 1));
}

Vector2 Sprite::FrameSize() const {
    if (!HasFrames()) return {0, 0};
    // pełna klatka (także przycięta w atlasie) - obszar trafienia nie skacze między klatkami
    return sheet_->FrameInfo(SheetFrame()).FullSize();
}

bool Sprite::Advance(const SpriteAnimation& clip) {
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
    const SpriteAnimation& clip = ActiveAnimation();
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

Vector2 Sprite::Pivot() const {
    const Vector2 frame = FrameSize();
    return {origin_.x + anchor_.x * frame.x, origin_.y + anchor_.y * frame.y};
}

bool Sprite::Contains(Vector2 point) const {
    const auto local = TryToLocal(point);
    const Vector2 frame = FrameSize();
    const Vector2 pivot = Pivot();
    return local && ::CheckCollisionPointRec(*local, {-pivot.x, -pivot.y, frame.x, frame.y});
}

void Sprite::Draw(PrimitiveRenderer& renderer) const {
    if (!HasFrames()) return;
    const SpriteFrame& frame = sheet_->FrameInfo(SheetFrame());
    Rectangle source = frame.area;
    // przycięta klatka leży w pełnej klatce w miejscu offset; odbicie odbija też to położenie
    const Vector2 full = frame.FullSize();
    Vector2 offset = frame.offset;
    if (flipX_) offset.x = full.x - frame.offset.x - source.width;
    if (flipY_) offset.y = full.y - frame.offset.y - source.height;
    // ujemny wymiar źródła odbija obraz (DrawTextureRec)
    if (flipX_) source.width = -source.width;
    if (flipY_) source.height = -source.height;
    const Vector2 pivot = Pivot();
    renderer.Blit(*sheet_->Bitmap(), source, {offset.x - pivot.x, offset.y - pivot.y}, color_);
}

} // namespace visual
