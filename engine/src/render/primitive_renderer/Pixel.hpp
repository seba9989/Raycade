/**
 * @file Pixel.hpp
 * @brief Wspólne funkcje pomocnicze implementacji PrimitiveRenderer (nagłówek prywatny).
 * @details Nie jest częścią API - leży w src/, więc gra go nie widzi.
 */
#pragma once

#include <cmath>

namespace pixel {

/**
 * @brief Zaokrągla współrzędną do najbliższego piksela.
 * @param value Współrzędna zmiennoprzecinkowa.
 * @return Numer piksela (połówki zaokrąglane od zera, jak std::lround).
 */
inline int Round(float value) {
    return static_cast<int>(std::lround(value));
}

} // namespace pixel
