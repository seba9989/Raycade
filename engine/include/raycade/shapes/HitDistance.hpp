/**
 * @file HitDistance.hpp
 * @brief Wspólna tolerancja trafień kursorem dla kształtów bez pola powierzchni.
 */
#pragma once

namespace shapes {

/**
 * @brief Odległość w pikselach, z jakiej kliknięcie trafia w punkt albo odcinek.
 * @details Używana przez Point, Points, Line, Lines i otwarty Polyline - ich obrys ma
 * grubość jednego piksela, więc bez tolerancji trudno byłoby w nie trafić.
 */
inline constexpr int kHitDistance = 4;

} // namespace shapes
