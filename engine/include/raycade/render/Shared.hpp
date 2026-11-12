/**
 * @file Shared.hpp
 * @brief Zamiana wyniku Load() na zasób współdzielony przez std::shared_ptr.
 */
#pragma once

#include <memory>
#include <optional>
#include <utility>

/**
 * @brief Przenosi wczytany zasób do std::shared_ptr; brak wartości daje nullptr.
 * @details Zasoby GPU (ShaderProgram, Typeface, Bitmap) wczytuje się przez Load(), które zwraca
 * std::optional, a obiekty sceny przyjmują je jako std::shared_ptr. Ta funkcja łączy oba kroki:
 * @code{.cpp}
 * auto wave = Shared(ShaderProgram::Load("", "assets/wave.fs")); // nullptr przy błędzie
 * if (!wave) engine.Log(LogLevel::Warning, "brak shadera fali");
 * scene.Spawn<visual::RenderGroup>().Shader(wave);
 * @endcode
 * Wynik konwertuje się niejawnie na `std::shared_ptr<const T>` (Typeface, Bitmap).
 * @tparam T Typ zasobu.
 * @param resource Wynik Load().
 * @return Zasób współdzielony albo nullptr.
 */
template <typename T> std::shared_ptr<T> Shared(std::optional<T>&& resource) {
    if (!resource) return nullptr;
    return std::make_shared<T>(std::move(*resource));
}
