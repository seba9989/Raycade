/**
 * @file Drawable.hpp
 * @brief Rysowanie jednego obiektu oraz porządkowanie obiektów według warstw.
 */
#pragma once

#include <raylib.h>

#include <algorithm>
#include <vector>

#include "../render/PrimitiveRenderer.hpp"

/**
 * @brief Obiekt, którego własny kształt można narysować.
 * @details Macierz i rysowanie dzieci obsługuje Entity.
 */
class Drawable {
  public:
    virtual ~Drawable() = default;

    /**
     * @brief Resetuje pędzel i rysuje własny kształt obiektu.
     * @details Implementacje Draw() podają rendererowi współrzędne lokalne. Entity nakłada
     * macierz i osobno wywołuje Render() dla dzieci, więc każde zaczyna z czystym pędzlem.
     * @param renderer Renderer prymitywów; metoda ustawia jego cel na ekran.
     */
    virtual void Render(PrimitiveRenderer& renderer) const {
        renderer.Reset().TargetScreen();
        // raylib nie ma stosu trybów mieszania, więc tryb dotyczy tylko własnego Draw()
        if (blend_ != BLEND_ALPHA) ::BeginBlendMode(blend_);
        Draw(renderer);
        if (blend_ != BLEND_ALPHA) ::EndBlendMode();
    }

    /**
     * @brief Pobiera warstwę rysowania wśród obiektów tego samego rodzica.
     * @return Numer warstwy; większa wartość oznacza rysowanie później.
     */
    int Layer() const { return layer_; }

    /**
     * @brief Ustawia sposób mieszania kolorów z tym, co już narysowano.
     * @details Dotyczy tylko własnego rysunku obiektu, bez dzieci. Przykłady: BLEND_ADDITIVE
     * (światło, ogień - kolory się sumują), BLEND_MULTIPLIED (cień), BLEND_ALPHA (domyślny).
     * @param mode Tryb raylib (BLEND_*).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Blend(this Self& self, int mode) {
        self.blend_ = mode;
        return self;
    }

    /** @brief Zwraca tryb mieszania kolorów (BLEND_*). */
    int Blend() const { return blend_; }

    /**
     * @brief Ustawia warstwę wśród obiektów tego samego rodzica.
     * @param layer Numer warstwy.
     * @return Ten sam obiekt jako jego konkretny typ (deducing this, C++23).
     */
    template <typename Self> Self& Layer(this Self& self, int layer) {
        self.layer_ = layer;
        return self;
    }

    /**
     * @brief Ustawia kolor rysowania.
     * @param color Kolor.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Paint(this Self& self, Color color) {
        self.color_ = color;
        return self;
    }

    /**
     * @brief Włącza lub wyłącza wypełnienie figury.
     * @details Działa w Rect, Triangle, Circle, Ellipse i Polygon; pozostałe kształty
     * je ignorują.
     * @param value Czy figura ma być wypełniona.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Fill(this Self& self, bool value = true) {
        self.filled_ = value;
        return self;
    }

    /** @brief Podaje kolor figury ustawiony przez Paint(). */
    Color Paint() const { return color_; }

    /** @brief Sprawdza, czy figura jest wypełniona (Fill()). */
    bool IsFilled() const { return filled_; }

    /**
     * @brief Ustawia obiekty od najniższej warstwy, zachowując kolejność dodania.
     * @param objects Obiekty o wspólnym rodzicu.
     * @return Wskaźniki w kolejności rysowania.
     */
    static std::vector<const Drawable*> ByLayer(std::vector<const Drawable*> objects) {
        std::stable_sort(objects.begin(), objects.end(), [](const Drawable* a, const Drawable* b) {
            return a->Layer() < b->Layer();
        });
        return objects;
    }

  protected:
    /** @brief Rysuje sam obiekt bez dzieci. */
    virtual void Draw(PrimitiveRenderer& renderer) const = 0;

    Color color_ = BLACK;     /**< Kolor figury. */
    bool filled_ = false;     /**< Czy figura jest wypełniona. */
    int layer_ = 0;           /**< Warstwa wśród rodzeństwa. */
    int blend_ = BLEND_ALPHA; /**< Tryb mieszania kolorów. */
};
