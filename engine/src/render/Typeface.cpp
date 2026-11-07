/**
 * @file Typeface.cpp
 * @brief Atlasy glifów dla każdego rozmiaru, dokładanie znaków, układ wierszy i rysowanie.
 */

#include <raycade/render/Typeface.hpp>

#include <rlgl.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <utility>

namespace {

/** @brief Odstęp wokół glifu w atlasie (jak w raylib) - zapobiega przenikaniu sąsiadów. */
constexpr int kGlyphPadding = 4;
/** @brief Najwięcej atlasów w pamięci; najdawniej używany jest usuwany. */
constexpr std::size_t kMaxAtlases = 16;

/**
 * @brief Rasteryzuje glify (LoadFontData z raylib 5.x i 6.x).
 * @details raylib 6 pomija znaki, których nie ma w pliku czcionki, i zwraca liczbę glifów
 * przez dodatkowy parametr - dlatego glif szuka się po GlyphInfo::value, a nie po pozycji na
 * liście znaków.
 * @param count Liczba zwróconych glifów.
 */
GlyphInfo* LoadGlyphs(
    const std::vector<unsigned char>& data,
    int pixelSize,
    const std::vector<int>& codepoints,
    int* count
) {
#if RAYLIB_VERSION_MAJOR >= 6
    return ::LoadFontData(
        data.data(), static_cast<int>(data.size()), pixelSize, codepoints.data(),
        static_cast<int>(codepoints.size()), FONT_DEFAULT, count
    );
#else
    *count = static_cast<int>(codepoints.size());
    // const_cast: raylib 5 przyjmuje int* bez const, ale tablicy nie zmienia
    return ::LoadFontData(
        data.data(), static_cast<int>(data.size()), pixelSize, const_cast<int*>(codepoints.data()),
        *count, FONT_DEFAULT
    );
#endif
}

/** @brief Dekoduje UTF-8; błędny bajt daje '?'. */
std::vector<int> Decode(std::string_view text) {
    std::vector<int> result;
    result.reserve(text.size());
    for (std::size_t i = 0; i < text.size();) {
        const auto byte = static_cast<unsigned char>(text[i]);
        int length = byte < 0x80           ? 1
                     : (byte >> 5) == 0x6  ? 2
                     : (byte >> 4) == 0xE  ? 3
                     : (byte >> 3) == 0x1E ? 4
                                           : 0;
        if (length == 0 || i + static_cast<std::size_t>(length) > text.size()) {
            result.push_back('?');
            ++i;
            continue;
        }
        int codepoint = length == 1 ? byte : byte & (0x7F >> length);
        for (int k = 1; k < length; ++k)
            codepoint = (codepoint << 6) |
                        (static_cast<unsigned char>(text[i + static_cast<std::size_t>(k)]) & 0x3F);
        result.push_back(codepoint);
        i += static_cast<std::size_t>(length);
    }
    return result;
}

/** @brief Przekształcenie 2D: x' = a x + c y + tx, y' = b x + d y + ty. */
struct Affine {
    float a = 1, b = 0, c = 0, d = 1, tx = 0, ty = 0;
};

/**
 * @brief Bieżące przekształcenie rysowania: macierz obiektu (stos rlgl) i kamera.
 * @details rlgl mnoży wierzchołki przez transform (stos rlPushMatrix) na CPU, a modelview
 * (kamera z BeginMode2D) w shaderze - punkt na ekranie to modelview * transform * punkt.
 */
Affine CurrentTransform() {
    const Matrix t = ::rlGetMatrixTransform();
    const Matrix v = ::rlGetMatrixModelview();
    return {
        v.m0 * t.m0 + v.m4 * t.m1,           v.m1 * t.m0 + v.m5 * t.m1,
        v.m0 * t.m4 + v.m4 * t.m5,           v.m1 * t.m4 + v.m5 * t.m5,
        v.m0 * t.m12 + v.m4 * t.m13 + v.m12, v.m1 * t.m12 + v.m5 * t.m13 + v.m13,
    };
}

/**
 * @brief Rozmiar atlasu dla rozmiaru na ekranie (zaokrąglony w górę).
 * @details Do 20 px dokładnie (mały tekst najbardziej traci na skalowaniu), wyżej co 4, 8
 * i 16 px. Animowana skala nie tworzy wtedy nowego atlasu w każdej klatce, a glif
 * zmniejszony o kilka procent wciąż jest ostry.
 */
int AtlasSize(float pixelSize) {
    const int size = std::clamp(static_cast<int>(std::ceil(pixelSize - 0.05f)), 4, 256);
    const auto up = [size](int step) { return (size + step - 1) / step * step; };
    if (size > 128) return up(16);
    if (size > 48) return up(8);
    if (size > 20) return up(4);
    return size;
}

/** @brief Wiersz ułożonego tekstu: zakres znaków [begin, end) i szerokość. */
struct Line {
    std::size_t begin = 0; /**< Pierwszy znak. */
    std::size_t end = 0;   /**< Za ostatnim znakiem. */
    float width = 0;       /**< Szerokość w pikselach. */
};

/**
 * @brief Dzieli tekst na wiersze: na '\n' i - gdy wrap > 0 - na spacjach przed przekroczeniem
 * szerokości (słowo dłuższe niż wiersz jest dzielone między znakami).
 */
std::vector<Line> Layout(
    const std::vector<int>& codepoints,
    const std::vector<float>& advances,
    float spacing,
    float wrap
) {
    const auto width = [&](std::size_t from, std::size_t to) {
        float result = 0;
        for (std::size_t i = from; i < to; ++i)
            result += advances[i];
        return to > from ? result + spacing * static_cast<float>(to - from - 1) : 0.0f;
    };
    std::vector<Line> lines;
    const auto push = [&](std::size_t from, std::size_t to) {
        // spacje na końcu łamanego wiersza nie liczą się do szerokości (wyrównanie)
        std::size_t trimmed = to;
        while (wrap > 0 && trimmed > from && codepoints[trimmed - 1] == ' ')
            --trimmed;
        lines.push_back({from, to, width(from, trimmed)});
    };

    std::size_t start = 0;
    while (true) {
        const auto newline = std::find(
            codepoints.begin() + static_cast<std::ptrdiff_t>(start), codepoints.end(), '\n'
        );
        const auto end = static_cast<std::size_t>(newline - codepoints.begin());
        if (wrap <= 0) {
            push(start, end);
        } else {
            std::size_t line = start;
            std::size_t space = end; // ostatnia spacja w wierszu (end - brak)
            float current = 0;
            for (std::size_t i = start; i < end; ++i) {
                float add = advances[i] + (i > line ? spacing : 0.0f);
                if (i > line && codepoints[i] != ' ' && current + add > wrap) {
                    if (space != end && space > line) {
                        push(line, space);
                        line = space + 1;
                    } else {
                        push(line, i);
                        line = i;
                    }
                    space = end;
                    current = width(line, i);
                    add = advances[i] + (i > line ? spacing : 0.0f);
                }
                if (codepoints[i] == ' ') space = i;
                current += add;
            }
            push(line, end);
        }
        if (end >= codepoints.size()) break;
        start = end + 1;
    }
    return lines;
}

} // namespace

std::optional<Typeface> Typeface::Load(const std::string& path, TypefaceOptions options) {
    std::string extension =
        ::GetFileExtension(path.c_str()) ? ::GetFileExtension(path.c_str()) : "";
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (extension != ".ttf" && extension != ".otf") {
        ::TraceLog(LOG_WARNING, "FONT: [%s] obslugiwane sa tylko pliki TTF i OTF", path.c_str());
        return std::nullopt;
    }

    int size = 0;
    unsigned char* raw = ::LoadFileData(path.c_str(), &size);
    if (raw == nullptr) return std::nullopt;
    Typeface result;
    result.data_.assign(raw, raw + size);
    ::UnloadFileData(raw);

    // próbna rasteryzacja jednego znaku: czy to poprawny plik czcionki (bez okna)
    int probed = 0;
    GlyphInfo* glyphs = LoadGlyphs(result.data_, 16, {'A'}, &probed);
    if (glyphs == nullptr || probed <= 0) {
        if (glyphs != nullptr) ::UnloadFontData(glyphs, probed);
        ::TraceLog(LOG_WARNING, "FONT: [%s] nieprawidlowy plik czcionki", path.c_str());
        return std::nullopt;
    }
    ::UnloadFontData(glyphs, probed);

    result.extension_ = std::move(extension);
    result.smooth_ = options.smooth;
    std::vector<int> wanted;
    for (int c = 32; c < 127; ++c)
        wanted.push_back(c);
    const std::vector<int> extra = Decode(std::string{kPolishLetters} + options.characters);
    wanted.insert(wanted.end(), extra.begin(), extra.end());
    for (int codepoint : wanted)
        if (codepoint >= 32 && result.known_.insert(codepoint).second)
            result.codepoints_.push_back(codepoint);
    return result;
}

const Typeface& Typeface::Default() {
    static const Typeface typeface;
    return typeface;
}

Typeface::~Typeface() {
    ReleaseAll();
}

Typeface::Typeface(Typeface&& other) noexcept
    : data_(std::move(other.data_)), extension_(std::move(other.extension_)),
      smooth_(other.smooth_), codepoints_(std::move(other.codepoints_)),
      known_(std::move(other.known_)), atlases_(std::move(other.atlases_)), clock_(other.clock_) {
    other.data_.clear();
    other.codepoints_.clear();
    other.known_.clear();
    other.atlases_.clear();
}

Typeface& Typeface::operator=(Typeface&& other) noexcept {
    if (this != &other) {
        ReleaseAll();
        data_ = std::exchange(other.data_, {});
        extension_ = std::exchange(other.extension_, {});
        smooth_ = other.smooth_;
        codepoints_ = std::exchange(other.codepoints_, {});
        known_ = std::exchange(other.known_, {});
        atlases_ = std::exchange(other.atlases_, {});
        clock_ = other.clock_;
    }
    return *this;
}

float Typeface::DefaultSpacing(float size) const {
    if (!IsDefault()) return 0.0f; // TTF/OTF: odstępy są już w glifach
    return std::max(1.0f, std::floor(size / 10.0f));
}

Typeface::Atlas Typeface::Build(int pixelSize) const {
    Atlas atlas;
    ::Font& font = atlas.font;
    font.baseSize = pixelSize;
    font.glyphPadding = kGlyphPadding;
    font.glyphs = LoadGlyphs(data_, pixelSize, codepoints_, &font.glyphCount);
    if (font.glyphs == nullptr || font.glyphCount <= 0) {
        if (font.glyphs != nullptr) ::UnloadFontData(font.glyphs, font.glyphCount);
        font.glyphs = nullptr;
        font.glyphCount = 0;
        return atlas;
    }
    for (int i = 0; i < font.glyphCount; ++i)
        atlas.glyphs.try_emplace(font.glyphs[i].value, i);
    atlas.pending =
        ::GenImageFontAtlas(font.glyphs, &font.recs, font.glyphCount, pixelSize, kGlyphPadding, 0);
    // obrazy pojedynczych glifów nie są potrzebne - rysujemy z atlasu
    for (int i = 0; i < font.glyphCount; ++i) {
        ::UnloadImage(font.glyphs[i].image);
        font.glyphs[i].image = {};
    }
    Upload(atlas);
    return atlas;
}

void Typeface::Upload(Atlas& atlas) const {
    if (atlas.pending.data == nullptr || !::IsWindowReady()) return;
    atlas.font.texture = ::LoadTextureFromImage(atlas.pending);
    ::SetTextureFilter(
        atlas.font.texture, smooth_ ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT
    );
    ::UnloadImage(atlas.pending);
    atlas.pending = {};
}

void Typeface::Release(Atlas& atlas) {
    ::Font& font = atlas.font;
    if (font.glyphs != nullptr) ::UnloadFontData(font.glyphs, font.glyphCount);
    if (font.recs != nullptr) ::MemFree(font.recs);
    // tekstura wymaga żywego kontekstu OpenGL (przed CloseWindow)
    if (font.texture.id != 0 && ::IsWindowReady()) {
        // atlas mógł być użyty w tej klatce - jego glify czekają w batchu rlgl i muszą trafić
        // na ekran, zanim tekstura zniknie (jej numer może dostać nowa tekstura)
        ::rlDrawRenderBatchActive();
        ::UnloadTexture(font.texture);
    }
    if (atlas.pending.data != nullptr) ::UnloadImage(atlas.pending);
    atlas = {};
}

void Typeface::ReleaseAll() const {
    for (auto& [size, atlas] : atlases_)
        Release(atlas);
    atlases_.clear();
}

const Typeface::Atlas& Typeface::AtlasFor(float pixelSize, bool needTexture) const {
    if (IsDefault()) {
        // font domyślny raylib powstaje przy InitWindow(), więc pobieramy go przy użyciu
        static Atlas fallback;
        fallback.font = ::GetFontDefault();
        return fallback;
    }
    const int size = AtlasSize(pixelSize);
    auto it = atlases_.find(size);
    if (it == atlases_.end()) {
        if (atlases_.size() >= kMaxAtlases) {
            const auto oldest = std::min_element(
                atlases_.begin(), atlases_.end(),
                [](const auto& x, const auto& y) { return x.second.used < y.second.used; }
            );
            Release(oldest->second);
            atlases_.erase(oldest);
        }
        it = atlases_.emplace(size, Build(size)).first;
    }
    it->second.used = ++clock_;
    if (needTexture) Upload(it->second);
    return it->second;
}

const ::Font& Typeface::Raw(float pixelSize) const {
    return AtlasFor(pixelSize, true).font;
}

void Typeface::EnsureGlyphs(const std::vector<int>& codepoints) const {
    if (IsDefault()) return;
    const std::size_t before = codepoints_.size();
    for (int codepoint : codepoints)
        if (codepoint >= 32 && known_.insert(codepoint).second) codepoints_.push_back(codepoint);
    if (codepoints_.size() == before) return;
    // raylib nie dokłada glifów do gotowego atlasu - nowe znaki to nowe atlasy (rzadko)
    ::TraceLog(
        LOG_INFO, "FONT: dolozono znaki (%d), atlasy zostana zbudowane od nowa",
        static_cast<int>(codepoints_.size() - before)
    );
    ReleaseAll();
}

int Typeface::GlyphIndex(const Atlas& atlas, int codepoint) const {
    if (IsDefault()) return atlas.font.glyphs ? ::GetGlyphIndex(atlas.font, codepoint) : -1;
    // znak, którego nie ma w pliku czcionki (raylib 6 go pomija) - jak w raylib: '?'
    if (const auto it = atlas.glyphs.find(codepoint); it != atlas.glyphs.end()) return it->second;
    const auto fallback = atlas.glyphs.find('?');
    return fallback != atlas.glyphs.end() ? fallback->second : -1;
}

Vector2 Typeface::Measure(std::string_view text, const TextStyle& style) const {
    if (text.empty()) return {0, style.size};
    const std::vector<int> codepoints = Decode(text);
    EnsureGlyphs(codepoints);
    const Atlas& atlas = AtlasFor(style.size, false);
    const ::Font& font = atlas.font;
    if (font.glyphs == nullptr || font.baseSize <= 0) return {0, style.size};

    const float scale = style.size / static_cast<float>(font.baseSize);
    std::vector<float> advances(codepoints.size(), 0.0f);
    for (std::size_t i = 0; i < codepoints.size(); ++i) {
        const int index = GlyphIndex(atlas, codepoints[i]);
        if (index < 0) continue;
        const int advance = font.glyphs[index].advanceX;
        advances[i] = (advance != 0 ? static_cast<float>(advance) : font.recs[index].width) * scale;
    }
    const std::vector<Line> lines = Layout(codepoints, advances, style.spacing, style.wrapWidth);
    float width = 0;
    for (const Line& line : lines)
        width = std::max(width, line.width);
    const float height =
        style.size + static_cast<float>(lines.size() - 1) * style.size * style.lineHeight;
    return {width, height};
}

void Typeface::Draw(
    std::string_view text,
    Vector2 position,
    const TextStyle& style,
    Color color
) const {
    if (text.empty()) return;
    const std::vector<int> codepoints = Decode(text);
    EnsureGlyphs(codepoints);

    // rozmiar atlasu = rozmiar tekstu na ekranie (skala obiektu, rodziców i kamery)
    const Affine m = CurrentTransform();
    float screenScale = std::max(std::hypot(m.a, m.b), std::hypot(m.c, m.d));
    if (!(screenScale > 0.0f)) screenScale = 1.0f;
    const Atlas& atlas = AtlasFor(style.size * screenScale, true);
    const ::Font& font = atlas.font;
    if (font.glyphs == nullptr || font.texture.id == 0 || font.baseSize <= 0) return;

    const float scale = style.size / static_cast<float>(font.baseSize);
    std::vector<int> indices(codepoints.size(), -1);
    std::vector<float> advances(codepoints.size(), 0.0f);
    for (std::size_t i = 0; i < codepoints.size(); ++i) {
        const int index = GlyphIndex(atlas, codepoints[i]);
        indices[i] = index;
        if (index < 0) continue;
        const int advance = font.glyphs[index].advanceX;
        advances[i] = (advance != 0 ? static_cast<float>(advance) : font.recs[index].width) * scale;
    }
    const std::vector<Line> lines = Layout(codepoints, advances, style.spacing, style.wrapWidth);
    float block = style.wrapWidth;
    if (block <= 0)
        for (const Line& line : lines)
            block = std::max(block, line.width);

    // bez obrotu i pochylenia glify trafiają w całe piksele ekranu - ostre krawędzie
    const bool snap = std::fabs(m.b) < 1e-4f && std::fabs(m.c) < 1e-4f && std::fabs(m.a) > 1e-4f &&
                      std::fabs(m.d) > 1e-4f;
    const float pad = static_cast<float>(font.glyphPadding);
    for (std::size_t l = 0; l < lines.size(); ++l) {
        const Line& line = lines[l];
        float x = position.x;
        if (style.align == TextAlign::Center) x += (block - line.width) / 2;
        if (style.align == TextAlign::Right) x += block - line.width;
        const float y = position.y + static_cast<float>(l) * style.size * style.lineHeight;
        for (std::size_t i = line.begin; i < line.end; ++i) {
            const int index = indices[i];
            const int codepoint = codepoints[i];
            if (index >= 0 && codepoint != ' ' && codepoint != '\t') {
                const GlyphInfo& glyph = font.glyphs[index];
                const Rectangle rec = font.recs[index];
                const Rectangle source{
                    rec.x - pad, rec.y - pad, rec.width + 2 * pad, rec.height + 2 * pad
                };
                Rectangle target{
                    x + (static_cast<float>(glyph.offsetX) - pad) * scale,
                    y + (static_cast<float>(glyph.offsetY) - pad) * scale, source.width * scale,
                    source.height * scale
                };
                if (snap) {
                    target.x = (std::round(m.a * target.x + m.tx) - m.tx) / m.a;
                    target.y = (std::round(m.d * target.y + m.ty) - m.ty) / m.d;
                }
                ::DrawTexturePro(font.texture, source, target, {0, 0}, 0.0f, color);
            }
            x += advances[i] + style.spacing;
        }
    }
}
