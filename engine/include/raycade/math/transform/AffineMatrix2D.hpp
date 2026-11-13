/**
 * @file AffineMatrix2D.hpp
 * @brief Przekształcenie afiniczne punktów 2D.
 */
#pragma once

#include <raylib.h>

#include <array>
#include <cmath>
#include <optional>

/**
 * @brief Macierz przekształcenia lokalnych współrzędnych na współrzędne rodzica.
 * @details Dla punktu (x, y) wynikiem jest (a*x + c*y + tx, b*x + d*y + ty).
 */
struct AffineMatrix2D {
    float a = 1.0f;
    float b = 0.0f;
    float c = 0.0f;
    float d = 1.0f;
    float tx = 0.0f;
    float ty = 0.0f;

    /** @brief Tworzy macierz przesunięcia o podany wektor. */
    static AffineMatrix2D Translation(Vector2 offset) {
        AffineMatrix2D result;
        result.tx = offset.x;
        result.ty = offset.y;
        return result;
    }

    /** @brief Tworzy macierz obrotu wokół początku układu o kąt w radianach. */
    static AffineMatrix2D Rotation(float radians) {
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        return {cosine, sine, -sine, cosine, 0.0f, 0.0f};
    }

    /** @brief Tworzy macierz skalowania osi X i Y wokół początku układu. */
    static AffineMatrix2D Scaling(Vector2 factors) {
        return {factors.x, 0.0f, 0.0f, factors.y, 0.0f, 0.0f};
    }

    /** @brief Przelicza punkt lokalny na układ rodzica. */
    Vector2 Apply(Vector2 point) const {
        return {a * point.x + c * point.y + tx, b * point.x + d * point.y + ty};
    }

    /**
     * @brief Przelicza punkt z układu rodzica na układ lokalny.
     * @return Punkt lokalny albo std::nullopt, gdy wyznacznik macierzy jest
     * nieokreślony lub zbyt bliski zeru, by bezpiecznie odwrócić macierz.
     */
    std::optional<Vector2> InverseApply(Vector2 point) const {
        const float determinant = a * d - b * c;
        if (!std::isfinite(determinant) || std::fabs(determinant) <= 1e-8f) return std::nullopt;
        const float x = point.x - tx;
        const float y = point.y - ty;
        return Vector2{(d * x - c * y) / determinant, (-b * x + a * y) / determinant};
    }

    /** @brief Łączy macierze: najpierw działa other, potem bieżąca macierz. */
    /**
     * @brief Macierz odwrotna (przekształcenie z powrotem).
     * @return Odwrotność albo std::nullopt dla macierzy osobliwej (np. skala 0).
     */
    std::optional<AffineMatrix2D> Inverse() const {
        const float determinant = a * d - b * c;
        if (!std::isfinite(determinant) || std::fabs(determinant) <= 1e-8f) return std::nullopt;
        const float ia = d / determinant;
        const float ib = -b / determinant;
        const float ic = -c / determinant;
        const float id = a / determinant;
        return AffineMatrix2D{ia, ib, ic, id, -(ia * tx + ic * ty), -(ib * tx + id * ty)};
    }

    AffineMatrix2D operator*(const AffineMatrix2D& other) const {
        // każde pole wyniku w osobnym wierszu
        // clang-format off
        return {a * other.a + c * other.b,
                b * other.a + d * other.b,
                a * other.c + c * other.d,
                b * other.c + d * other.d,
                a * other.tx + c * other.ty + tx,
                b * other.tx + d * other.ty + ty};
        // clang-format on
    }

    /** @brief Macierz 4x4 w układzie kolumnowym wymaganym przez rlgl. */
    std::array<float, 16> RlglMatrix() const {
        // dwie kolumny macierzy 4x4 na wiersz
        // clang-format off
        return {a, b, 0.0f, 0.0f, c, d, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f, tx, ty, 0.0f, 1.0f};
        // clang-format on
    }
};
