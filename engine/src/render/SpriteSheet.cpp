/**
 * @file SpriteSheet.cpp
 * @brief Siatka klatek, opis animacji i ich parametry.
 */
#include <raycade/render/SpriteSheet.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>

#include <nlohmann/json.hpp>

namespace {

/** @brief Klatka z samym prostokątem (bez przycięcia, czasu i nazwy). */
SpriteFrame FrameOf(Rectangle area) {
    SpriteFrame frame;
    frame.area = area;
    return frame;
}

} // namespace

int SpriteAnimation::Length() const {
    return std::max(1, frames.empty() ? count : static_cast<int>(frames.size()));
}

int SpriteAnimation::SheetFrame(int index) const {
    index = std::clamp(index, 0, Length() - 1);
    if (!frames.empty()) return frames[static_cast<std::size_t>(index)];
    return first + index;
}

float SpriteAnimation::Duration(int index) const {
    if (index >= 0 && static_cast<std::size_t>(index) < durations.size() &&
        durations[static_cast<std::size_t>(index)] > 0.0f)
        return durations[static_cast<std::size_t>(index)];
    return fps > 0.0f ? 1.0f / fps : 0.0f;
}

SpriteSheet::SpriteSheet(std::shared_ptr<const ::Bitmap> bitmap) : bitmap_(std::move(bitmap)) {
    if (bitmap_ && !bitmap_->IsEmpty())
        frames_.push_back(FrameOf(
            Rectangle{
                0.0f, 0.0f, static_cast<float>(bitmap_->Width()),
                static_cast<float>(bitmap_->Height())
            }
        ));
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
        sheet.frames_.push_back(FrameOf(
            Rectangle{
                margin.x + static_cast<float>(i % columns) * (frame.x + spacing.x),
                margin.y + static_cast<float>(i / columns) * (frame.y + spacing.y), frame.x, frame.y
            }
        ));
    return sheet;
}

const SpriteFrame& SpriteSheet::FrameInfo(int index) const {
    static const SpriteFrame empty;
    if (frames_.empty()) return empty;
    return frames_[static_cast<std::size_t>(std::clamp(index, 0, FrameCount() - 1))];
}

int SpriteSheet::FindFrame(const std::string& name) const {
    for (std::size_t i = 0; i < frames_.size(); ++i)
        if (frames_[i].name == name) return static_cast<int>(i);
    return -1;
}

SpriteSheet& SpriteSheet::Animation(
    const std::string& name,
    int first,
    int count,
    float fps,
    Playback mode
) {
    SpriteAnimation clip;
    clip.first = first;
    clip.count = count;
    clip.fps = fps;
    clip.mode = mode;
    return Animation(name, std::move(clip));
}

SpriteSheet& SpriteSheet::Animation(
    const std::string& name,
    std::vector<int> frames,
    float fps,
    Playback mode
) {
    SpriteAnimation clip;
    clip.frames = std::move(frames);
    clip.fps = fps;
    clip.mode = mode;
    return Animation(name, std::move(clip));
}

SpriteAnimation* SpriteSheet::EditAnimation(const std::string& name, const char* function) {
    const auto it = animations_.find(name);
    if (it != animations_.end()) return &it->second;
    ::TraceLog(LOG_WARNING, "SpriteSheet::%s: nieznana animacja \"%s\"", function, name.c_str());
    return nullptr;
}

SpriteSheet& SpriteSheet::Durations(const std::string& name, std::vector<float> seconds) {
    if (SpriteAnimation* clip = EditAnimation(name, "Durations"))
        clip->durations = std::move(seconds);
    return *this;
}

SpriteSheet& SpriteSheet::Event(const std::string& name, int frame, std::string event) {
    if (SpriteAnimation* clip = EditAnimation(name, "Event"))
        clip->events.emplace_back(frame, std::move(event));
    return *this;
}

const SpriteAnimation* SpriteSheet::FindAnimation(const std::string& name) const {
    const auto it = animations_.find(name);
    return it == animations_.end() ? nullptr : &it->second;
}

// ---------------------------------------------------------------- JSON (Aseprite, TexturePacker)

namespace {

/** @brief JSON z zachowaną kolejnością kluczy - klatki "Hash" muszą zostać w kolejności pliku. */
using Json = nlohmann::ordered_json;

/** @brief Prostokąt {x, y, w, h} z JSON. */
Rectangle ReadRect(const Json& node) {
    return {
        node.value("x", 0.0f), node.value("y", 0.0f), node.value("w", 0.0f), node.value("h", 0.0f)
    };
}

/** @brief Klatka z opisu frames[i]. */
SpriteFrame ReadFrame(const Json& node, std::string name, bool& rotated) {
    SpriteFrame frame;
    frame.name = std::move(name);
    frame.area = ReadRect(node.at("frame"));
    if (node.value("rotated", false)) rotated = true;
    if (const auto source = node.find("spriteSourceSize"); source != node.end())
        frame.offset = {source->value("x", 0.0f), source->value("y", 0.0f)};
    if (const auto size = node.find("sourceSize"); size != node.end())
        frame.size = {size->value("w", 0.0f), size->value("h", 0.0f)};
    frame.duration = node.value("duration", 0.0f) / 1000.0f; // Aseprite: milisekundy
    return frame;
}

/** @brief Liczba powtórzeń tagu: Aseprite zapisuje ją jako tekst ("3"), inne programy - liczbę. */
int ReadRepeat(const Json& tag) {
    const auto repeat = tag.find("repeat");
    if (repeat == tag.end()) return 0;
    if (repeat->is_number_integer()) return repeat->get<int>();
    if (repeat->is_string()) {
        const std::string text = repeat->get<std::string>();
        return text.empty() ? 0 : std::max(0, std::atoi(text.c_str()));
    }
    return 0;
}

/** @brief Nazwa animacji z nazwy klatki: "run_01.png" -> "run", "hero 3.aseprite" -> "hero". */
std::string GroupName(const std::string& frame) {
    std::string name = std::filesystem::path{frame}.stem().string();
    while (!name.empty() && std::isdigit(static_cast<unsigned char>(name.back())))
        name.pop_back();
    while (!name.empty() &&
           (name.back() == '_' || name.back() == '-' || name.back() == ' ' || name.back() == '.'))
        name.pop_back();
    return name.empty() ? frame : name;
}

/** @brief Uzupełnia czasy klatek animacji z czasów zapisanych w klatkach arkusza. */
void FillDurations(
    SpriteAnimation& clip,
    const std::vector<int>& frames,
    const SpriteSheet& sheet
) {
    float total = 0;
    clip.durations.clear();
    for (int frame : frames) {
        clip.durations.push_back(sheet.FrameInfo(frame).duration);
        total += clip.durations.back();
    }
    // fps jako średnia - przydaje się, gdy któraś klatka nie ma czasu
    if (total > 0) clip.fps = static_cast<float>(frames.size()) / total;
    if (total <= 0) clip.durations.clear();
}

} // namespace

std::optional<SpriteSheet> SpriteSheet::Load(
    const std::string& path,
    const BitmapLoader& loadBitmap
) {
    char* raw = ::LoadFileText(path.c_str());
    if (raw == nullptr) return std::nullopt; // przyczyna w logu raylib (FILEIO)
    const std::string text = raw;
    ::UnloadFileText(raw);

    const Json document = Json::parse(text, nullptr, false);
    if (document.is_discarded() || !document.is_object()) {
        ::TraceLog(LOG_WARNING, "SPRITESHEET: [%s] nieprawidlowy JSON", path.c_str());
        return std::nullopt;
    }

    SpriteSheet sheet;
    bool rotated = false;
    try {
        const Json& frames = document.at("frames");
        if (frames.is_array()) {
            for (const Json& frame : frames)
                sheet.frames_.push_back(ReadFrame(frame, frame.value("filename", ""), rotated));
        } else if (frames.is_object()) {
            for (const auto& [name, frame] : frames.items())
                sheet.frames_.push_back(ReadFrame(frame, name, rotated));
        }

        const Json meta = document.value("meta", Json::object());
        const std::string image = meta.value("image", "");
        if (image.empty()) {
            ::TraceLog(LOG_WARNING, "SPRITESHEET: [%s] brak meta.image", path.c_str());
            return std::nullopt;
        }
        // obraz leży obok pliku JSON (ścieżka w meta.image jest względna)
        const std::string imagePath =
            (std::filesystem::path{path}.parent_path() / image).lexically_normal().string();
        sheet.bitmap_ = loadBitmap ? loadBitmap(imagePath) : Shared(::Bitmap::Load(imagePath));
        if (!sheet.bitmap_) {
            ::TraceLog(
                LOG_WARNING, "SPRITESHEET: [%s] nie mozna wczytac obrazu \"%s\"", path.c_str(),
                imagePath.c_str()
            );
            return std::nullopt;
        }

        const Json tags = meta.value("frameTags", Json::array());
        for (const Json& tag : tags) {
            const int from =
                std::clamp(tag.value("from", 0), 0, std::max(0, sheet.FrameCount() - 1));
            const int to =
                std::clamp(tag.value("to", from), from, std::max(0, sheet.FrameCount() - 1));
            std::vector<int> order;
            for (int i = from; i <= to; ++i)
                order.push_back(i);
            const std::string direction = tag.value("direction", "forward");
            if (direction == "reverse" || direction == "pingpong_reverse")
                std::reverse(order.begin(), order.end());
            const bool pingPong = direction == "pingpong" || direction == "pingpong_reverse";

            SpriteAnimation clip;
            clip.mode = pingPong ? Playback::PingPong : Playback::Loop;
            if (const int repeat = ReadRepeat(tag); repeat > 0) {
                // N odtworzeń i stop: rozpisujemy klatki (tam i z powrotem bez powtarzania
                // skrajnych) i odtwarzamy raz
                std::vector<int> cycle = order;
                if (pingPong && order.size() > 2)
                    cycle.insert(cycle.end(), order.rbegin() + 1, order.rend() - 1);
                std::vector<int> expanded;
                for (int r = 0; r < repeat; ++r)
                    expanded.insert(expanded.end(), cycle.begin(), cycle.end());
                if (pingPong && order.size() > 1) expanded.push_back(order.front());
                order = std::move(expanded);
                clip.mode = Playback::Once;
            }
            clip.frames = order;
            FillDurations(clip, order, sheet);
            sheet.animations_[tag.value("name", "")] = std::move(clip);
        }

        // bez tagów (np. TexturePacker): animacje z nazw klatek, w kolejności pierwszego
        // wystąpienia
        if (tags.empty()) {
            std::vector<std::pair<std::string, std::vector<int>>> groups;
            for (int i = 0; i < sheet.FrameCount(); ++i) {
                if (sheet.frames_[static_cast<std::size_t>(i)].name.empty()) continue;
                const std::string group =
                    GroupName(sheet.frames_[static_cast<std::size_t>(i)].name);
                auto it = std::find_if(groups.begin(), groups.end(), [&](const auto& g) {
                    return g.first == group;
                });
                if (it == groups.end()) {
                    groups.emplace_back(group, std::vector<int>{});
                    it = groups.end() - 1;
                }
                it->second.push_back(i);
            }
            for (auto& [group, order] : groups) {
                SpriteAnimation clip;
                clip.frames = order;
                FillDurations(clip, order, sheet);
                sheet.animations_[group] = std::move(clip);
            }
        }
    } catch (const nlohmann::json::exception& error) {
        // brak wymaganego pola (np. "frame") albo pole złego typu
        ::TraceLog(LOG_WARNING, "SPRITESHEET: [%s] %s", path.c_str(), error.what());
        return std::nullopt;
    }

    if (rotated)
        ::TraceLog(
            LOG_WARNING,
            "SPRITESHEET: [%s] obrocone klatki nie sa obslugiwane - wylacz obracanie przy "
            "eksporcie",
            path.c_str()
        );
    if (sheet.frames_.empty())
        ::TraceLog(LOG_WARNING, "SPRITESHEET: [%s] brak klatek", path.c_str());
    return sheet;
}
