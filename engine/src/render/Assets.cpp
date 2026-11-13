/**
 * @file Assets.cpp
 * @brief Pamięć podręczna zasobów z plików, zasoby zastępcze i zwalnianie nieużywanych.
 */
#include <raycade/render/Assets.hpp>

#include <filesystem>
#include <utility>

Assets& Assets::Local() {
    static Assets local;
    return local;
}

Assets& Assets::Logger(LogFunction log) {
    log_ = std::move(log);
    return *this;
}

Assets& Assets::Directory(std::string directory) {
    directory_ = std::move(directory);
    return *this;
}

std::string Assets::Path(const std::string& name) const {
    const std::filesystem::path path{name};
    if (path.is_absolute() || directory_.empty()) return path.lexically_normal().string();
    return (std::filesystem::path{directory_} / path).lexically_normal().string();
}

void Assets::Report(
    const std::string& key,
    std::string_view kind,
    const std::string& path,
    std::string_view fallback
) {
    // jeden wpis na plik - gra może prosić o brakujący zasób co klatkę
    if (!failed_.insert(key).second) return;
    const std::string message = "ASSETS: nie mozna wczytac " + std::string(kind) + " \"" + path +
                                "\" - " + std::string(fallback);
    if (log_)
        log_(LogLevel::Warning, message);
    else
        ::TraceLog(LOG_WARNING, "%s", message.c_str());
}

std::shared_ptr<const ::Bitmap> Assets::Bitmap(const std::string& name) {
    // klucz to pełna ścieżka: "hero.png" i obraz wskazany przez "hero.json" to ten sam plik
    const std::string path = Path(name);
    if (const auto it = bitmaps_.find(path); it != bitmaps_.end()) return it->second;
    if (failed_.contains(path)) return Missing();

    auto bitmap = Shared(::Bitmap::Load(path));
    if (!bitmap) {
        Report(path, "bitmapy", path, "szachownica");
        return Missing();
    }
    bitmaps_.emplace(path, bitmap);
    return bitmap;
}

std::shared_ptr<const SpriteSheet> Assets::Sheet(const std::string& name) {
    const std::string path = Path(name);
    if (const auto it = sheets_.find(path); it != sheets_.end()) return it->second;
    if (!missingSheet_) missingSheet_ = std::make_shared<const SpriteSheet>(Missing());
    if (failed_.contains(path)) return missingSheet_;

    // obraz arkusza też przez pamięć podręczną - wspólny z Bitmap() dla tego samego pliku
    const auto loadImage = [this](const std::string& image) -> std::shared_ptr<const ::Bitmap> {
        auto bitmap = Bitmap(image);
        return bitmap->IsLoaded() ? bitmap : nullptr;
    };
    auto sheet = Shared(SpriteSheet::Load(path, loadImage));
    if (!sheet) {
        Report(path, "arkusza", path, "szachownica");
        return missingSheet_;
    }
    sheets_.emplace(path, sheet);
    return sheet;
}

std::shared_ptr<const Typeface> Assets::Font(
    const std::string& name,
    const TypefaceOptions& options
) {
    // ustawienia są częścią klucza - inny zestaw znaków albo filtr to inna czcionka
    const std::string key =
        Path(name) + '\x1f' + options.characters + (options.smooth ? "" : "\x1fpoint");
    if (const auto it = fonts_.find(key); it != fonts_.end()) return it->second;
    if (!defaultTypeface_) defaultTypeface_ = std::make_shared<const Typeface>();
    if (failed_.contains(key)) return defaultTypeface_;

    const std::string path = Path(name);
    auto font = Shared(Typeface::Load(path, options));
    if (!font) {
        Report(key, "czcionki", path, "font domyslny");
        return defaultTypeface_;
    }
    fonts_.emplace(key, font);
    return font;
}

std::shared_ptr<ShaderProgram> Assets::Shader(const std::string& fragment) {
    return Shader("", fragment);
}

std::shared_ptr<ShaderProgram> Assets::Shader(
    const std::string& vertex,
    const std::string& fragment
) {
    // pusty napis zostaje pusty - oznacza domyślny etap raylib, a nie katalog zasobów
    const std::string vertexPath = vertex.empty() ? "" : Path(vertex);
    const std::string fragmentPath = fragment.empty() ? "" : Path(fragment);
    const std::string key = vertexPath + '\x1f' + fragmentPath;
    if (const auto it = shaders_.find(key); it != shaders_.end()) return it->second;
    if (!missingShader_) missingShader_ = std::make_shared<ShaderProgram>();
    if (failed_.contains(key)) return missingShader_;

    const std::string name = vertexPath.empty() || fragmentPath.empty()
                                 ? vertexPath + fragmentPath
                                 : vertexPath + "\" + \"" + fragmentPath;
    // bez kontekstu OpenGL raylib nie skompiluje shadera (i może się wyłożyć)
    if (!::IsWindowReady()) {
        Report(key, "shadera (okno nie jest otwarte)", name, "bez efektu");
        return missingShader_;
    }
    auto shader = Shared(ShaderProgram::Load(vertexPath, fragmentPath));
    if (!shader) {
        Report(key, "shadera", name, "bez efektu");
        return missingShader_;
    }
    shaders_.emplace(key, shader);
    return shader;
}

std::shared_ptr<const AudioClip> Assets::Clip(const std::string& name) {
    const std::string path = Path(name);
    if (const auto it = sounds_.find(path); it != sounds_.end()) return it->second;
    if (!silentClip_) silentClip_ = std::make_shared<const AudioClip>();
    if (failed_.contains(path)) return silentClip_;

    auto clip = Shared(AudioClip::Load(path));
    if (!clip) {
        Report(path, "dzwieku", path, "cisza");
        return silentClip_;
    }
    sounds_.emplace(path, clip);
    return clip;
}

std::shared_ptr<const ::Bitmap> Assets::Missing() {
    if (missing_) return missing_;
    constexpr int kSize = 16;
    constexpr int kCell = 4;
    ::Bitmap bitmap{kSize, kSize, BLACK};
    for (int y = 0; y < kSize; ++y)
        for (int x = 0; x < kSize; ++x)
            if ((x / kCell + y / kCell) % 2 == 0) bitmap.Pixel(x, y, MAGENTA);
    bitmap.Placeholder(); // IsLoaded() == false - gra może sprawdzić, czy plik się wczytał
    missing_ = std::make_shared<const ::Bitmap>(std::move(bitmap));
    return missing_;
}

std::size_t Assets::Purge() {
    std::size_t released = 0;
    const auto purge = [&released](auto& cache) {
        // use_count() == 1: zasób trzyma już tylko pamięć podręczna
        released +=
            std::erase_if(cache, [](const auto& entry) { return entry.second.use_count() == 1; });
    };
    // najpierw arkusze: trzymają swoje bitmapy, więc te zwolnią się w tym samym przebiegu
    purge(sheets_);
    purge(bitmaps_);
    purge(fonts_);
    purge(shaders_);
    purge(sounds_); // grający głos trzyma swój klip, więc zostaje
    failed_.clear();
    return released;
}

Assets& Assets::Clear() {
    sheets_.clear();
    missingSheet_.reset();
    bitmaps_.clear();
    fonts_.clear();
    shaders_.clear();
    sounds_.clear();
    failed_.clear();
    missing_.reset();
    defaultTypeface_.reset();
    missingShader_.reset();
    silentClip_.reset();
    return *this;
}
