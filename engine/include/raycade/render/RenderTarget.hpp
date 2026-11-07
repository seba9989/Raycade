/**
 * @file RenderTarget.hpp
 * @brief Tekstura na karcie graficznej, do której można rysować (render do tekstury).
 */
#pragma once

#include <raylib.h>

/**
 * @brief Tekstura GPU jako cel rysowania, z obsługą zagnieżdżania.
 * @details Między Begin() a End() wszystko, co rysuje raylib i PrimitiveRenderer (na ekran),
 * trafia do tej tekstury, w jej własnym układzie: (0, 0) to lewy górny róg, bez kamery
 * i bez przekształceń obiektów, wewnątrz których wywołano Begin().
 *
 * W odróżnieniu od samego BeginTextureMode() z raylib cele można zagnieżdżać: End() wraca do
 * poprzedniego celu (innej tekstury, np. tekstury skalowania okna, albo ekranu) i przywraca
 * kamerę oraz przekształcenia, które obowiązywały przed Begin().
 * @code{.cpp}
 * RenderTarget minimap(200, 150);
 * minimap.Begin();
 * minimap.Clear(DARKGRAY);
 * ::DrawCircle(100, 75, 10, RED);
 * minimap.End();
 * minimap.Draw({20, 20}); // w bieżącym celu, z bieżącą kamerą
 * @endcode
 *
 * Teksturę tworzy i zwalnia się przy otwartym oknie (kontekst OpenGL). Obiekt można przenosić,
 * ale nie kopiować.
 */
class RenderTarget {
  public:
    /** @brief Pusty cel (bez tekstury). */
    RenderTarget() = default;

    /**
     * @brief Tworzy teksturę o podanym rozmiarze (wymaga otwartego okna).
     * @param width Szerokość w pikselach.
     * @param height Wysokość w pikselach.
     */
    RenderTarget(int width, int height);

    /** @brief Zwalnia teksturę (tylko przy otwartym oknie). */
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    /** @brief Przenosi teksturę z innego obiektu. */
    RenderTarget(RenderTarget&& other) noexcept;
    /** @brief Przenosi teksturę z innego obiektu, zwalniając własną. */
    RenderTarget& operator=(RenderTarget&& other) noexcept;

    /** @brief Czy tekstura istnieje. */
    bool IsValid() const { return target_.id != 0; }
    /** @brief Szerokość tekstury w pikselach. */
    int Width() const { return target_.texture.width; }
    /** @brief Wysokość tekstury w pikselach. */
    int Height() const { return target_.texture.height; }

    /**
     * @brief Wybiera filtr przy skalowaniu tekstury.
     * @param smooth false - ostre piksele (domyślnie), true - wygładzanie dwuliniowe.
     * @return Ten sam cel.
     */
    RenderTarget& Smooth(bool smooth);

    /**
     * @brief Udostępnia teksturę z zawartością.
     * @details Uwaga: tekstura renderowana jest w OpenGL odwrócona w pionie - przy własnym
     * rysowaniu trzeba podać ujemną wysokość źródła. Draw() robi to samo.
     */
    const Texture2D& Texture() const { return target_.texture; }

    /**
     * @brief Zaczyna rysowanie do tekstury.
     * @details Zapamiętuje bieżący cel, kamerę (macierz widoku), projekcję i przekształcenia
     * obiektów, a w teksturze zaczyna od czystego układu. Każde Begin() wymaga End().
     */
    void Begin() const;

    /** @brief Kończy rysowanie do tekstury i przywraca stan sprzed Begin(). */
    void End() const;

    /**
     * @brief Czyści teksturę kolorem (także przezroczystym, np. BLANK).
     * @details Między Begin() a End() czyści tylko tę teksturę; poza nimi robi własne
     * Begin()/End().
     * @param color Kolor czyszczenia.
     * @return Ten sam cel.
     */
    const RenderTarget& Clear(Color color) const;

    /**
     * @brief Rysuje zawartość tekstury w bieżącym celu (ekran albo inna tekstura).
     * @param position Lewy górny róg, w bieżącym układzie (z kamerą i przekształceniami).
     * @param tint Kolor mnożący (WHITE - bez zmian; z alfą - półprzezroczystość).
     */
    void Draw(Vector2 position, Color tint = WHITE) const;

    /**
     * @brief Rysuje zawartość tekstury przeskalowaną do prostokąta.
     * @param destination Prostokąt docelowy w bieżącym układzie.
     * @param tint Kolor mnożący.
     */
    void Draw(Rectangle destination, Color tint = WHITE) const;

    /** @brief Czy trwa rysowanie do tej tekstury (jest na szczycie stosu). */
    bool IsActive() const;

    /** @brief Liczba zagnieżdżonych celów (0 - rysowanie na ekran). */
    static int Depth();

  private:
    /** @brief Zwalnia teksturę, jeśli kontekst OpenGL jeszcze istnieje. */
    void Release();

    RenderTexture2D target_{}; /**< Tekstura raylib. */
};
