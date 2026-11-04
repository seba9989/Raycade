/**
 * @file Group.hpp
 * @brief Kształt Group - niewidoczny rodzic do grupowania obiektów.
 */
#pragma once

#include "../Entity.hpp"

namespace shapes {

/**
 * @brief Niewidoczny rodzic służący do grupowania i przesuwania dzieci.
 * @details Przydaje się np. dla postaci złożonej z kilku figur albo układu obiektów
 * poruszających się razem. Dziedziczy domyślne Contains(), więc sama grupa
 * nie ma obszaru reagowania na mysz.
 */
class Group : public Entity {
  protected:
    /** @brief Grupa nie rysuje niczego - widoczne są tylko jej dzieci. */
    void Draw(PrimitiveRenderer&) const override {}
};

} // namespace shapes
