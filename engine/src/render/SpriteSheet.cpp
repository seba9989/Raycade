/**
 * @file SpriteSheet.cpp
 * @brief Siatka klatek, opis animacji i ich parametry.
 */
#include <raycade/render/SpriteSheet.hpp>

#include <algorithm>
#include <cmath>

int SpriteClip::Length() const {
    return std::max(1, frames.empty() ? count : static_cast<int>(frames.size()));
}

int SpriteClip::SheetFrame(int index) const {
    index = std::clamp(index, 0, Length() - 1);
    if (!frames.empty()) return frames[static_cast<std::size_t>(index)];
    return first + index;
}

float SpriteClip::Duration(int index) const {
    if (index >= 0 && static_cast<std::size_t>(index) < durations.size() &&
        durations[static_cast<std::size_t>(index)] > 0.0f)
        return durations[static_cast<std::size_t>(index)];
    return fps > 0.0f ? 1.0f / fps : 0.0f;
}

SpriteSheet::SpriteSheet(std::shared_ptr<const ::Bitmap> bitmap) : bitmap_(std::move(bitmap)) {
    if (bitmap_ && !bitmap_->IsEmpty())
        frames_.push_back(
            {0.0f, 0.0f, static_cast<float>(bitmap_->Width()),
             static_cast<float>(bitmap_->Height())}
        );
}

SpriteSheet SpriteSheet::Grid(
    std::shared_ptr<const ::Bitmap> bitmap,
    Vector2 frameSize,
    Vector2 margin,
    Vector2 spacing,
    int count
) {
    if (!bitmap || bitmap->IsEmpty()) return SpriteSheet{std::move(bitmap)};
    const float width = static_cast<float>(bitmap->Width());
    const float height = static_cast<float>(bitmap->Height());
    if (frameSize.x <= 0 || frameSize.y <= 0) return SpriteSheet{std::move(bitmap)};

    // klatka większa od bitmapy jest przycinana - tak działał dawny Sprite::Sheet()
    const Vector2 frame{
        std::min(frameSize.x, width - margin.x), std::min(frameSize.y, height - margin.y)
    };
    SpriteSheet sheet;
    sheet.bitmap_ = std::move(bitmap);
    if (frame.x <= 0 || frame.y <= 0) return sheet;

    // n klatek zajmuje n * frame + (n - 1) * spacing pikseli
    const auto fit = [](float space, float size, float gap) {
        return std::max(1, static_cast<int>(std::floor((space + gap) / (size + gap))));
    };
    const int columns = fit(width - margin.x, frame.x, spacing.x);
    const int rows = fit(height - margin.y, frame.y, spacing.y);
    const int total = count > 0 ? std::min(count, columns * rows) : columns * rows;
    sheet.frames_.reserve(static_cast<std::size_t>(total));
    for (int i = 0; i < total; ++i)
        sheet.frames_.push_back(
            {margin.x + static_cast<float>(i % columns) * (frame.x + spacing.x),
             margin.y + static_cast<float>(i / columns) * (frame.y + spacing.y), frame.x, frame.y}
        );
    return sheet;
}

Rectangle SpriteSheet::Frame(int index) const {
    if (frames_.empty()) return {0, 0, 0, 0};
    return frames_[static_cast<std::size_t>(std::clamp(index, 0, FrameCount() - 1))];
}

SpriteSheet& SpriteSheet::Clip(
    const std::string& name,
    int first,
    int count,
    float fps,
    Playback mode
) {
    SpriteClip clip;
    clip.first = first;
    clip.count = count;
    clip.fps = fps;
    clip.mode = mode;
    return Clip(name, std::move(clip));
}

SpriteSheet& SpriteSheet::Clip(
    const std::string& name,
    std::vector<int> frames,
    float fps,
    Playback mode
) {
    SpriteClip clip;
    clip.frames = std::move(frames);
    clip.fps = fps;
    clip.mode = mode;
    return Clip(name, std::move(clip));
}

SpriteClip* SpriteSheet::EditClip(const std::string& name, const char* function) {
    const auto it = clips_.find(name);
    if (it != clips_.end()) return &it->second;
    ::TraceLog(LOG_WARNING, "SpriteSheet::%s: nieznana animacja \"%s\"", function, name.c_str());
    return nullptr;
}

SpriteSheet& SpriteSheet::Durations(const std::string& name, std::vector<float> seconds) {
    if (SpriteClip* clip = EditClip(name, "Durations")) clip->durations = std::move(seconds);
    return *this;
}

SpriteSheet& SpriteSheet::Event(const std::string& name, int frame, std::string event) {
    if (SpriteClip* clip = EditClip(name, "Event"))
        clip->events.emplace_back(frame, std::move(event));
    return *this;
}

const SpriteClip* SpriteSheet::FindClip(const std::string& name) const {
    const auto it = clips_.find(name);
    return it == clips_.end() ? nullptr : &it->second;
}
