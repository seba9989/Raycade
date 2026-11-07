/**
 * @file ui.hpp
 * @brief Nagłówek zbiorczy elementów interfejsu z przestrzeni nazw @ref ui.
 * @details Dołączany przez `<raycade>`. Elementy UI korzystają z tych samych klas bazowych co
 * kształty (Entity, Focusable), ale nie są figurami geometrycznymi, dlatego mają osobną
 * przestrzeń nazw i katalog.
 *
 * | Typ                 | Co robi                                                       |
 * |---------------------|---------------------------------------------------------------|
 * | @ref ui::TextField  | jednowierszowe pole tekstowe z kursorem, schowkiem i fokusem  |
 */
#pragma once

#include <raycade/ui/TextField.hpp> // IWYU pragma: export
