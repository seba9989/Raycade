/**
 * @file Transformable.hpp
 * @brief Wspólna implementacja pozycji, przesuwania, obracania i skalowania obiektów sceny.
 */
#pragma once

#include "AffineMatrix2D.hpp"

/**
 * @brief Obiekt z lokalną macierzą transformacji względem rodzica.
 * @details Metody zmieniające transformację używają jawnego parametru obiektu z C++23
 * (`this Self& self`, tzw. deducing this). Self to typ, na którym metodę wywołano, więc
 * `rect.Rotate(...)` zwraca `shapes::Rect&` i łańcuch wywołań można kontynuować setterami
 * konkretnego kształtu.
 *
 * Dodatni kąt obraca zgodnie z ruchem wskazówek zegara, bo oś Y ekranu jest skierowana w dół.
 */
class Transformable {
  public:
    virtual ~Transformable() = default;

    /**
     * @brief Pobiera pozycję lokalną względem rodzica.
     * @return Punkt, w który trafia lokalne (0, 0) obiektu.
     */
    Vector2 Position() const { return {transform_.tx, transform_.ty}; }

    /**
     * @brief Ustawia pozycję względem rodzica, bez zmiany obrotu i skali.
     * @param position Nowa pozycja lokalna.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Position(this Self& self, Vector2 position) {
        self.transform_.tx = position.x;
        self.transform_.ty = position.y;
        return self;
    }

    /**
     * @brief Przesuwa obiekt o wektor w układzie rodzica.
     * @param offset Przesunięcie.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Translate(this Self& self, Vector2 offset) {
        self.transform_.tx += offset.x;
        self.transform_.ty += offset.y;
        return self;
    }

    /**
     * @brief Obraca obiekt i jego dzieci wokół punktu w układzie rodzica.
     * @param radians Kąt obrotu w radianach.
     * @param pivot Środek obrotu w układzie rodzica.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Rotate(this Self& self, float radians, Vector2 pivot) {
        self.transform_ = AffineMatrix2D::Translation(pivot) * AffineMatrix2D::Rotation(radians) *
                          AffineMatrix2D::Translation({-pivot.x, -pivot.y}) * self.transform_;
        return self;
    }

    /**
     * @brief Obraca obiekt wokół jego własnej pozycji.
     * @param radians Kąt obrotu w radianach.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Rotate(this Self& self, float radians) {
        return self.Rotate(radians, self.Position());
    }

    /**
     * @brief Skaluje obiekt i jego dzieci względem punktu w układzie rodzica.
     * @param factors Mnożniki skali osi X i Y.
     * @param pivot Środek skalowania w układzie rodzica.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Scale(this Self& self, Vector2 factors, Vector2 pivot) {
        self.transform_ = AffineMatrix2D::Translation(pivot) * AffineMatrix2D::Scaling(factors) *
                          AffineMatrix2D::Translation({-pivot.x, -pivot.y}) * self.transform_;
        return self;
    }

    /**
     * @brief Skaluje obiekt względem jego własnej pozycji.
     * @param factors Mnożniki skali osi X i Y.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Scale(this Self& self, Vector2 factors) {
        return self.Scale(factors, self.Position());
    }

  protected:
    AffineMatrix2D transform_; /**< Lokalna macierz obiektu. */
};
