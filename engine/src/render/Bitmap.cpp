/**
 * @file Bitmap.cpp
 * @brief Zarządzanie obrazem w RAM i teksturą aktualizowaną przy rysowaniu.
 */

#include <raycade/render/Bitmap.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

/**
 * @details GenImageColor tworzy obraz w formacie R8G8B8A8, więc piksele to zwykła
 * tablica Color.
 */
Bitmap::Bitmap(int width, int height, Color background)
    : image_(::GenImageColor(width, height, background)) {
}

Bitmap::Bitmap(Image image) : image_(image) {
    if (image_.data != nullptr && image_.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
        ::ImageFormat(&image_, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
}

std::optional<Bitmap> Bitmap::Load(const std::string& path) {
    Image image = ::LoadImage(path.c_str());
    if (image.data == nullptr) return std::nullopt;
    return Bitmap{image};
}

bool Bitmap::Save(const std::string& path) const {
    return !IsEmpty() && ::ExportImage(image_, path.c_str());
}

Bitmap Bitmap::Region(Rectangle area) const {
    // obcięcie do bitmapy - ImageFromImage nie sprawdza zakresu
    const float left = std::max(0.0f, std::floor(area.x));
    const float top = std::max(0.0f, std::floor(area.y));
    const float right = std::min(static_cast<float>(Width()), std::floor(area.x + area.width));
    const float bottom = std::min(static_cast<float>(Height()), std::floor(area.y + area.height));
    if (IsEmpty() || right <= left || bottom <= top) return {};
    return Bitmap{::ImageFromImage(image_, {left, top, right - left, bottom - top})};
}

Bitmap::~Bitmap() {
    Release();
}

Bitmap::Bitmap(Bitmap&& other) noexcept
    : image_(std::exchange(other.image_, {})), texture_(std::exchange(other.texture_, {})),
      dirty_(other.dirty_), placeholder_(std::exchange(other.placeholder_, false)) {
}

Bitmap& Bitmap::operator=(Bitmap&& other) noexcept {
    if (this != &other) {
        Release();
        image_ = std::exchange(other.image_, {});
        texture_ = std::exchange(other.texture_, {});
        dirty_ = other.dirty_;
        placeholder_ = std::exchange(other.placeholder_, false);
    }
    return *this;
}

void Bitmap::Release() {
    // teksturę można zwolnić tylko przy żywym kontekście OpenGL (przed CloseWindow)
    if (texture_.id != 0 && ::IsWindowReady()) ::UnloadTexture(texture_);
    if (image_.data != nullptr) ::UnloadImage(image_);
    image_ = {};
    texture_ = {};
}

Color Bitmap::Pixel(int x, int y) const {
    if (!Contains(x, y)) return BLANK;
    return static_cast<const Color*>(image_.data)[y * Width() + x];
}

Bitmap& Bitmap::Pixel(int x, int y, Color color) {
    if (!Contains(x, y)) return *this;
    static_cast<Color*>(image_.data)[y * Width() + x] = color;
    dirty_ = true;
    return *this;
}

Bitmap& Bitmap::Clear(Color color) {
    ::ImageClearBackground(&image_, color);
    dirty_ = true;
    return *this;
}

const Texture2D& Bitmap::Texture() const {
    if (texture_.id == 0) {
        texture_ = ::LoadTextureFromImage(image_);
        dirty_ = false;
    } else if (dirty_) {
        ::UpdateTexture(texture_, image_.data);
        dirty_ = false;
    }
    return texture_;
}
