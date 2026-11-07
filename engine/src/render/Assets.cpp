/**
 * @file Assets.cpp
 * @brief Pamięć podręczna bitmap i czcionek, zasoby zastępcze i zwalnianie nieużywanych.
 */
#include <raycade/render/Assets.hpp>

#include <filesystem>
#include <utility>

Assets& Assets::Directory(std::string directory) {
    directory_ = std::move(directory);
    return *this;
}

std::string Assets::Path(const std::string& name) const {
    const std::filesystem::path path{name};
    if (path.is_absolute() || directory_.empty()) return name;
    return (std::filesystem::path{directory_} / path).string();
}

bool Assets::Failed(const std::string& key, const std::string& path) {
    // jeden wpis na plik - gra może prosić o brakujący zasób co klatkę
    if (failed_.insert(key).second)
        ::TraceLog(LOG_ERROR, "ASSETS: nie mozna wczytac \"%s\"", path.c_str());
    return true;
}

std::shared_ptr<const ::Bitmap> Assets::Bitmap(const std::string& name) {
    if (const auto it = bitmaps_.find(name); it != bitmaps_.end()) return it->second;
    if (failed_.contains(name)) return Missing();

    const std::string path = Path(name);
    auto bitmap = Shared(::Bitmap::Load(path));
    if (!bitmap) {
        Failed(name, path);
        return Missing();
    }
    bitmaps_.emplace(name, bitmap);
    return bitmap;
}

std::shared_ptr<const Typeface> Assets::Font(
    const std::string& name,
    int size,
    const std::string& extraCharacters,
    bool smooth
) {
    // parametry są częścią klucza - inny rozmiar albo zestaw znaków to inna czcionka
    const std::string key =
        name + '\x1f' + std::to_string(size) + '\x1f' + extraCharacters + (smooth ? "" : "\x1fp");
    if (const auto it = fonts_.find(key); it != fonts_.end()) return it->second;
    if (!defaultTypeface_) defaultTypeface_ = std::make_shared<const Typeface>();
    if (failed_.contains(key)) return defaultTypeface_;

    const std::string path = Path(name);
    auto font = Shared(Typeface::Load(path, size, extraCharacters, smooth));
    if (!font) {
        Failed(key, path);
        return defaultTypeface_;
    }
    fonts_.emplace(key, font);
    return font;
}

std::shared_ptr<const ::Bitmap> Assets::Missing() {
    if (missing_) return missing_;
    constexpr int kSize = 16;
    constexpr int kCell = 4;
    ::Bitmap bitmap{kSize, kSize, BLACK};
    for (int y = 0; y < kSize; ++y)
        for (int x = 0; x < kSize; ++x)
            if ((x / kCell + y / kCell) % 2 == 0) bitmap.Pixel(x, y, MAGENTA);
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
    purge(bitmaps_);
    purge(fonts_);
    failed_.clear();
    return released;
}

Assets& Assets::Clear() {
    bitmaps_.clear();
    fonts_.clear();
    failed_.clear();
    missing_.reset();
    defaultTypeface_.reset();
    return *this;
}
