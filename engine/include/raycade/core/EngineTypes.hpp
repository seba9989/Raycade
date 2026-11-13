/**
 * @file EngineTypes.hpp
 * @brief Typy ustawień silnika: poziom logu i tryb skalowania obrazu.
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
 * @brief Sposób dopasowania obrazu gry do okna (Engine::ScaleMode()).
 * @details Przy trybie innym niż None gra rysuje zawsze w stałej rozdzielczości
 * (Engine::Resolution()) do tekstury, która jest potem skalowana na okno. Współrzędne myszy
 * są przeliczane automatycznie, więc obiekty, kliknięcia i MousePosition() działają
 * w rozdzielczości gry niezależnie od rozmiaru okna.
 */
enum class Scaling {
    None,        /**< Bez skalowania - współrzędne gry to piksele okna (domyślnie). */
    Stretch,     /**< Rozciągnięcie na całe okno, bez zachowania proporcji. */
    Fit,         /**< Największa skala z zachowaniem proporcji; resztę wypełniają pasy. */
    PixelPerfect /**< Jak Fit, ale tylko całkowite skale (ostre piksele, większe pasy). */
};
