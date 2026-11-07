/**
 * @file ShaderProgram.cpp
 * @brief Wczytywanie, zwalnianie i uniformy shadera.
 */

#include <raycade/render/ShaderProgram.hpp>

#include <raycade/render/Bitmap.hpp>

#include <rlgl.h>

#include <utility>

namespace {

/** @brief Pusty napis oznacza w raylib domyślny etap shadera (nullptr). */
const char* OrNull(const std::string& text) {
    return text.empty() ? nullptr : text.c_str();
}

/** @brief Zwraca shader, gdy się skompilował; nieudany zwalnia od razu (destruktor). */
std::optional<ShaderProgram> Checked(ShaderProgram shader) {
    if (!shader.IsValid()) return std::nullopt;
    return shader;
}

} // namespace

std::optional<ShaderProgram> ShaderProgram::Load(const std::string& vertexPath,
                                                 const std::string& fragmentPath) {
    return Checked(ShaderProgram{::LoadShader(OrNull(vertexPath), OrNull(fragmentPath))});
}

std::optional<ShaderProgram> ShaderProgram::FromCode(const std::string& vertexCode,
                                                     const std::string& fragmentCode) {
    return Checked(ShaderProgram{::LoadShaderFromMemory(OrNull(vertexCode), OrNull(fragmentCode))});
}

ShaderProgram::~ShaderProgram() {
    Release();
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : shader_(std::exchange(other.shader_, {})), locations_(std::move(other.locations_)) {
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        Release();
        shader_ = std::exchange(other.shader_, {});
        locations_ = std::move(other.locations_);
    }
    return *this;
}

/**
 * @details Przy błędzie kompilacji raylib zwraca domyślny shader (o id domyślnego programu).
 * Taki "shader" działa, ale nie jest tym, o który proszono - dlatego nie liczy się jako poprawny.
 */
bool ShaderProgram::IsValid() const {
    return shader_.id != 0 && shader_.id != ::rlGetShaderIdDefault() && ::IsShaderValid(shader_);
}

void ShaderProgram::Release() {
    // shader można zwolnić tylko przy żywym kontekście OpenGL (przed CloseWindow);
    // UnloadShader() sam pomija domyślny program raylib
    if (shader_.id != 0 && ::IsWindowReady()) ::UnloadShader(shader_);
    shader_ = {};
    locations_.clear();
}

int ShaderProgram::Location(const std::string& name) const {
    if (shader_.id == 0) return -1;
    const auto it = locations_.find(name);
    if (it != locations_.end()) return it->second;
    const int location = ::GetShaderLocation(shader_, name.c_str());
    locations_.emplace(name, location);
    return location;
}

ShaderProgram& ShaderProgram::Set(const std::string& name, const void* value, int type) {
    const int location = Location(name);
    if (location >= 0) ::SetShaderValue(shader_, location, value, type);
    return *this;
}

ShaderProgram& ShaderProgram::Uniform(const std::string& name, float value) {
    return Set(name, &value, SHADER_UNIFORM_FLOAT);
}

ShaderProgram& ShaderProgram::Uniform(const std::string& name, int value) {
    return Set(name, &value, SHADER_UNIFORM_INT);
}

ShaderProgram& ShaderProgram::Uniform(const std::string& name, Vector2 value) {
    return Set(name, &value, SHADER_UNIFORM_VEC2);
}

ShaderProgram& ShaderProgram::Uniform(const std::string& name, Vector3 value) {
    return Set(name, &value, SHADER_UNIFORM_VEC3);
}

ShaderProgram& ShaderProgram::Uniform(const std::string& name, Vector4 value) {
    return Set(name, &value, SHADER_UNIFORM_VEC4);
}

ShaderProgram& ShaderProgram::Uniform(const std::string& name, Color value) {
    const Vector4 normalized = ::ColorNormalize(value);
    return Set(name, &normalized, SHADER_UNIFORM_VEC4);
}

ShaderProgram& ShaderProgram::Uniform(const std::string& name, const Texture2D& texture) {
    const int location = Location(name);
    if (location >= 0) ::SetShaderValueTexture(shader_, location, texture);
    return *this;
}

ShaderProgram& ShaderProgram::Uniform(const std::string& name, const Bitmap& bitmap) {
    return Uniform(name, bitmap.Texture());
}
