/**
 * @file ShaderProgram.hpp
 * @brief Shader GPU (GLSL) z automatycznym zwalnianiem i ustawianiem uniformów po nazwie.
 */
#pragma once

#include <raylib.h>

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>

#include <raycade/core/Shared.hpp> // IWYU pragma: export

class Bitmap;

/**
 * @brief Program shadera GPU: wierzchołków i fragmentów (GLSL).
 * @details Opakowanie raylib Shader: zwalnia shader w destruktorze, pamięta położenia
 * uniformów i ustawia je po nazwie. Klasa nazywa się ShaderProgram, bo nazwę Shader ma już
 * struktura raylib.
 *
 * Shader podpina się do obiektu (Drawable::Shader()), grupy (node::visual::RenderGroup - efekt na
 * całym rysunku dzieci) albo do całego obrazu gry (Engine::PostShader()). Obiekty dzielą
 * shader przez std::shared_ptr, więc jeden program może obsłużyć wiele obiektów.
 *
 * Shader wczytuje się po otwarciu okna (wymaga kontekstu OpenGL) - np. w konfiguracji sceny.
 * Plik najprościej pobrać przez pamięć podręczną zasobów (Assets::Shader()): nazwa jest
 * liczona od katalogu assets/, a brakujący plik daje pusty shader (rysunek bez efektu):
 * @code{.cpp}
 * auto wave = engine.Assets().Shader("wave.fs");
 * scene.Spawn<node::visual::RenderGroup>().Size({300, 200}).Shader(wave);
 * scene.OnUpdate([wave](float) {
 *     wave->Uniform("time", static_cast<float>(Engine::Get().Time()));
 * });
 * @endcode
 * Load() i FromCode() tworzą osobny program - np. gdy obiekty mają mieć różne wartości
 * uniformów: `Shared(ShaderProgram::Load("", engine.AssetPath("wave.fs")))`.
 */
class ShaderProgram {
  public:
    /** @brief Pusty shader (IsLoaded() zwraca false); obiekt z nim rysuje się zwyczajnie. */
    ShaderProgram() = default;

    /**
     * @brief Przejmuje shader raylib (np. z LoadShader()).
     * @param shader Shader; nie wolno go już zwalniać samodzielnie.
     */
    explicit ShaderProgram(::Shader shader) : shader_(shader) {}

    /**
     * @brief Wczytuje shader z plików GLSL.
     * @param vertexPath Plik shadera wierzchołków albo pusty napis (domyślny shader raylib).
     * @param fragmentPath Plik shadera fragmentów albo pusty napis (domyślny shader raylib).
     * @return Shader albo brak wartości przy błędzie kompilacji lub braku pliku (przyczynę
     * raylib zapisuje w logu) - tak jak Bitmap::Load() i Typeface::Load().
     */
    static std::optional<ShaderProgram> Load(
        const std::string& vertexPath,
        const std::string& fragmentPath
    );

    /**
     * @brief Kompiluje shader z kodu GLSL w napisach.
     * @param vertexCode Kod shadera wierzchołków albo pusty napis (domyślny).
     * @param fragmentCode Kod shadera fragmentów albo pusty napis (domyślny).
     * @return Shader albo brak wartości przy błędzie kompilacji.
     */
    static std::optional<ShaderProgram> FromCode(
        const std::string& vertexCode,
        const std::string& fragmentCode
    );

    /** @brief Zwalnia shader z karty graficznej. */
    ~ShaderProgram();

    /** @brief Shader to zasób GPU - nie można go kopiować (współdzieli się go przez shared_ptr). */
    ShaderProgram(const ShaderProgram&) = delete;
    /** @brief Shader to zasób GPU - nie można go kopiować. */
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    /** @brief Przenosi shader; źródło zostaje puste. */
    ShaderProgram(ShaderProgram&& other) noexcept;
    /** @brief Przenosi shader; poprzedni jest zwalniany. */
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    /** @brief Sprawdza, czy shader został poprawnie skompilowany i wczytany. */
    bool IsLoaded() const;

    /**
     * @brief Sprawdza, czy shader ma uniform o podanej nazwie.
     * @details Kompilator GLSL usuwa nieużywane uniformy - wtedy też zwraca false.
     * @param name Nazwa uniformu.
     * @return Czy uniform istnieje.
     */
    bool HasUniform(const std::string& name) const { return Location(name) >= 0; }

    /** @name Uniformy (brak uniformu o tej nazwie jest pomijany) */
    /** @{ */
    /** @brief Ustawia uniform `float`. */
    ShaderProgram& Uniform(const std::string& name, float value);
    /** @brief Ustawia uniform `float` (wygodna wersja dla literałów typu `0.5`). */
    ShaderProgram& Uniform(const std::string& name, double value) {
        return Uniform(name, static_cast<float>(value));
    }
    /** @brief Ustawia uniform `int` (także `bool` i `sampler2D` jako numer jednostki). */
    ShaderProgram& Uniform(const std::string& name, int value);
    /** @brief Ustawia uniform `vec2`. */
    ShaderProgram& Uniform(const std::string& name, Vector2 value);
    /** @brief Ustawia uniform `vec3`. */
    ShaderProgram& Uniform(const std::string& name, Vector3 value);
    /** @brief Ustawia uniform `vec4`. */
    ShaderProgram& Uniform(const std::string& name, Vector4 value);
    /** @brief Ustawia uniform `vec4` z koloru (składowe 0-255 przeliczone na 0-1). */
    ShaderProgram& Uniform(const std::string& name, Color value);
    /** @brief Ustawia uniform `sampler2D` na teksturę. */
    ShaderProgram& Uniform(const std::string& name, const Texture2D& texture);
    /** @brief Ustawia uniform `sampler2D` na teksturę bitmapy (wysyła ją na GPU, jeśli trzeba). */
    ShaderProgram& Uniform(const std::string& name, const Bitmap& bitmap);
    /** @} */

    /**
     * @name Tablice uniformów (brak uniformu o tej nazwie albo pusta tablica jest pomijana)
     * @details Ustawiają kolejne elementy tablicy GLSL od indeksu 0, np. `uniform vec4 lights[16];`
     * przyjmuje do 16 wartości - reszta tablicy zachowuje poprzednią zawartość, więc liczbę
     * użytych elementów zwykle przekazuje się osobnym uniformem `int`:
     * @code{.cpp}
     * std::vector<Vector4> lights = ...;
     * shader->Uniform("lights", lights).Uniform("lightCount", static_cast<int>(lights.size()));
     * @endcode
     */
    /** @{ */
    /** @brief Ustawia tablicę `float[]`. */
    ShaderProgram& Uniform(const std::string& name, std::span<const float> values);
    /** @brief Ustawia tablicę `int[]`. */
    ShaderProgram& Uniform(const std::string& name, std::span<const int> values);
    /** @brief Ustawia tablicę `vec2[]`. */
    ShaderProgram& Uniform(const std::string& name, std::span<const Vector2> values);
    /** @brief Ustawia tablicę `vec3[]`. */
    ShaderProgram& Uniform(const std::string& name, std::span<const Vector3> values);
    /** @brief Ustawia tablicę `vec4[]`. */
    ShaderProgram& Uniform(const std::string& name, std::span<const Vector4> values);
    /** @} */

    /** @brief Udostępnia shader raylib (np. do BeginShaderMode() we własnym rysowaniu). */
    const ::Shader& Raw() const { return shader_; }

  private:
    /** @brief Położenie uniformu (z pamięci podręcznej) albo -1. */
    int Location(const std::string& name) const;

    /** @brief Ustawia wartość uniformu typu SHADER_UNIFORM_*. */
    ShaderProgram& Set(const std::string& name, const void* value, int type);

    /** @brief Ustawia `count` elementów tablicy uniformów typu SHADER_UNIFORM_*. */
    ShaderProgram& SetArray(
        const std::string& name,
        const void* values,
        int type,
        std::size_t count
    );

    void Release();

    ::Shader shader_{}; /**< Shader raylib. */
    mutable std::unordered_map<std::string, int>
        locations_; /**< Pamięć podręczna położeń uniformów. */
};
