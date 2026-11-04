/**
 * @file Bitmap.hpp
 * @brief Klasa Bitmap - obraz w pamięci RAM z teksturą do wyświetlania.
 */
#pragma once

#include <raylib.h>

/**
 * @brief Obraz rastrowy w pamięci RAM z teksturą aktualizowaną przy rysowaniu.
 * @details W odróżnieniu od ekranu z bitmapy można odczytywać kolory pikseli, czego wymagają
 * algorytmy wypełniania przez spójność (boundary fill, flood fill). Na ekran trafia jako
 * tekstura, wysyłana na kartę graficzną dopiero przy wywołaniu Texture() i tylko wtedy,
 * gdy zawartość się zmieniła.
 * @code{.cpp}
 * Bitmap bitmap{400, 300, WHITE};
 * renderer.Target(bitmap).Paint(BLACK).Circle({200, 150}, 80).TargetScreen();
 * renderer.Blit(bitmap, {100, 100});
 * @endcode
 */
class Bitmap {
  public:
    /** @brief Tworzy pustą bitmapę (0 x 0). */
    Bitmap() = default;

    /**
     * @brief Tworzy bitmapę o jednolitym kolorze.
     * @param width Szerokość w pikselach.
     * @param height Wysokość w pikselach.
     * @param background Kolor początkowy.
     */
    Bitmap(int width, int height, Color background = BLANK);

    /** @brief Zwalnia obraz i ewentualną teksturę. */
    ~Bitmap();

    /** @brief Kopiowanie bitmapy jest zabronione. */
    Bitmap(const Bitmap&) = delete;
    /** @brief Przypisanie kopii bitmapy jest zabronione. */
    Bitmap& operator=(const Bitmap&) = delete;

    /**
     * @brief Przenosi zasoby z innej bitmapy.
     * @param other Bitmapa źródłowa, po przeniesieniu pusta.
     */
    Bitmap(Bitmap&& other) noexcept;

    /**
     * @brief Przenosi zasoby z innej bitmapy, zwalniając dotychczasowe.
     * @param other Bitmapa źródłowa, po przeniesieniu pusta.
     * @return Ta sama bitmapa.
     */
    Bitmap& operator=(Bitmap&& other) noexcept;

    /**
     * @brief Pobiera szerokość bitmapy.
     * @return Liczba pikseli w poziomie.
     */
    int Width() const { return image_.width; }

    /**
     * @brief Pobiera wysokość bitmapy.
     * @return Liczba pikseli w pionie.
     */
    int Height() const { return image_.height; }

    /**
     * @brief Sprawdza, czy piksel leży w granicach bitmapy.
     * @param x Współrzędna pozioma.
     * @param y Współrzędna pionowa.
     * @return true dla poprawnych współrzędnych.
     */
    bool Contains(int x, int y) const { return x >= 0 && y >= 0 && x < Width() && y < Height(); }

    /**
     * @brief Odczytuje kolor piksela.
     * @param x Współrzędna pozioma.
     * @param y Współrzędna pionowa.
     * @return Kolor piksela albo BLANK poza bitmapą.
     */
    Color Pixel(int x, int y) const;

    /**
     * @brief Ustawia kolor piksela; zapis poza bitmapą jest ignorowany.
     * @param x Współrzędna pozioma.
     * @param y Współrzędna pionowa.
     * @param color Nowy kolor.
     * @return Ta sama bitmapa.
     */
    Bitmap& Pixel(int x, int y, Color color);

    /**
     * @brief Wypełnia całą bitmapę jednym kolorem.
     * @param color Kolor wypełnienia.
     * @return Ta sama bitmapa.
     */
    Bitmap& Clear(Color color);

    /**
     * @brief Udostępnia obraz raylib do modyfikacji funkcjami ImageDraw*.
     * @details Oznacza bitmapę jako zmienioną, więc tekstura zostanie odświeżona.
     * @return Referencja do obrazu w RAM.
     */
    Image& Edit() {
        dirty_ = true;
        return image_;
    }

    /**
     * @brief Pobiera teksturę odpowiadającą aktualnej zawartości bitmapy.
     * @return Referencja do tekstury raylib.
     * @pre Okno musi być otwarte przed utworzeniem tekstury.
     */
    const Texture2D& Texture() const;

  private:
    /** @brief Zwalnia teksturę (tylko przy otwartym oknie) i obraz. */
    void Release();

    Image image_{};               /**< Piksele w RAM (format R8G8B8A8). */
    mutable Texture2D texture_{}; /**< Kopia na karcie graficznej, tworzona na żądanie. */
    mutable bool dirty_ = true;   /**< Czy tekstura wymaga odświeżenia. */
};
