/**
 * @file ClipStack.hpp
 * @brief Stos przycięć rysowania (scissor) zgodny z przekształceniami i RenderTarget.
 */
#pragma once

#include <raylib.h>

/**
 * @brief Przycinanie rysowania do prostokąta, z zagnieżdżaniem.
 * @details raylib ma jeden poziom BeginScissorMode() w pikselach bufora: nie uwzględnia
 * macierzy obiektów ani kamery, nie zagnieżdża się, a przy BeginTextureMode() zostaje włączony
 * i przycina także rysunek w teksturze. Ta klasa uzupełnia te braki:
 * - prostokąt podaje się w bieżącym układzie rlgl (lokalnym obiektu) - jest przeliczany przez
 *   macierz obiektów, kamerę i projekcję na piksele bieżącego bufora,
 * - kolejne przycięcia są przecinane z poprzednimi,
 * - RenderTarget otwiera osobny poziom: w teksturze nic nie jest przycinane przez przycięcia
 *   z zewnątrz, a po End() zewnętrzne przycięcie wraca.
 *
 * Przy obrocie przycinana jest obwiednia (prostokąt równoległy do osi ekranu) - scissor
 * w OpenGL nie potrafi przyciąć do obróconego prostokąta. Do dokładnego przycinania obróconej
 * grupy służy visual::RenderGroup.
 *
 * Zwykle wystarczy Drawable::Clip() - Entity samo woła Push() i Pop() przy rysowaniu.
 * @code{.cpp}
 * ClipStack::Push({0, 0, 200, 100});
 * renderer.Paint(RED).Filled().Circle({200, 100}, 80); // widać tylko ćwiartkę
 * ClipStack::Pop();
 * @endcode
 */
class ClipStack {
  public:
    /**
     * @brief Zaczyna przycinanie do prostokąta (przeciętego z bieżącym przycięciem).
     * @param area Prostokąt w bieżącym układzie rlgl (np. lokalnym obiektu).
     */
    static void Push(Rectangle area);

    /** @brief Kończy ostatnie przycięcie i przywraca poprzednie (albo wyłącza przycinanie). */
    static void Pop();

    /** @brief Sprawdza, czy na bieżącym poziomie (ekranie albo teksturze) coś jest przycinane. */
    static bool IsActive();

  private:
    friend class RenderTarget;

    /**
     * @brief Otwiera poziom tekstury: wyłącza przycinanie z zewnątrz.
     * @details Woła RenderTarget::Begin() po BeginTextureMode().
     * @param width Szerokość tekstury.
     * @param height Wysokość tekstury.
     */
    static void BeginTarget(int width, int height);

    /**
     * @brief Zamyka poziom tekstury i przywraca przycięcie poprzedniego poziomu.
     * @details Woła RenderTarget::End() po powrocie do poprzedniego celu.
     */
    static void EndTarget();
};
