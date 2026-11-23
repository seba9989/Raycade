/**
 * @file EngineTypes.hpp
 * @brief Typy ustawień silnika: poziom logu, tryb skalowania obrazu i proporcje.
 */
#pragma once

/** @brief Poziom komunikatu w logu silnika (Engine::Log()). */
enum class LogLevel {
    Debug,   /**< Szczegóły diagnostyczne (m.in. TRACE i DEBUG z raylib). */
    Info,    /**< Zwykła informacja, np. wczytanie sceny. */
    Warning, /**< Coś pominięto, ale program działa dalej. */
    Error    /**< Operacja się nie udała. */
};

/**
 * @brief Sposób rysowania obrazu gry przy dopasowaniu do okna (Engine::ScaleMode()).
 * @details Przy trybie innym niż None gra ma stały układ współrzędnych (Engine::ViewSize()),
 * a obraz jest skalowany na okno zgodnie z Engine::AspectMode(). Współrzędne myszy są
 * przeliczane automatycznie, więc obiekty, kliknięcia i MousePosition() działają w jednostkach
 * gry niezależnie od rozmiaru okna.
 */
enum class Scaling {
    None,    /**< Bez skalowania - współrzędne gry to piksele okna (domyślnie). */
    Canvas,  /**< Rysowanie w rozdzielczości okna, skalowany jest układ współrzędnych (ostry
                tekst i krawędzie). */
    Viewport /**< Rysowanie do tekstury o rozmiarze widocznego obszaru w jednostkach gry
                (1 jednostka = 1 piksel), skalowanej potem na okno - dla grafiki pikselowej. */
};

/**
 * @brief Zachowanie przy proporcjach okna innych niż Engine::ViewSize() (Engine::AspectMode()).
 * @details Początek układu zostaje w lewym górnym rogu ViewSize(). Tryby, które pokazują więcej
 * niż ViewSize(), powiększają obszar w prawo i w dół - jego rozmiar podaje
 * Engine::VisibleSize().
 */
enum class Aspect {
    Stretch,    /**< Rozciągnięcie na całe okno, bez zachowania proporcji. */
    Keep,       /**< Stałe proporcje; resztę okna wypełniają pasy (domyślnie). */
    KeepWidth,  /**< Stała szerokość; w wyższym oknie widać więcej w pionie, w szerszym - pasy. */
    KeepHeight, /**< Stała wysokość; w szerszym oknie widać więcej w poziomie, w wyższym - pasy. */
    Expand      /**< Bez pasów: w każdym kierunku widać więcej niż ViewSize(). */
};
