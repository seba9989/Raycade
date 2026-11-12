/**
 * @file Transformable.hpp
 * @brief Wspólna implementacja pozycji, przesuwania, obracania i skalowania obiektów sceny.
 */
#pragma once

#include <cmath>

#include <raycade/math/AffineMatrix2D.hpp>

/**
 * @brief Obiekt z lokalną macierzą transformacji względem rodzica.
 * @details Metody zmieniające transformację używają jawnego parametru obiektu z C++23
 * (`this Self& self`, tzw. deducing this). Self to typ, na którym metodę wywołano, więc
 * `rect.Rotate(...)` zwraca `shapes::Rect&` i łańcuch wywołań można kontynuować setterami
 * konkretnego kształtu.
 *
 * Każda z trzech składowych ma parę metod: rzeczownik ustawia albo podaje wartość
 * bezwzględną (Position(), Rotation(), Scale()), a czasownik zmienia bieżącą o podaną wartość
 * (Translate(), Rotate(), ScaleBy()). Czasowniki działają w układzie rodzica i mogą obracać
 * albo skalować wokół dowolnego punktu.
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
     * @brief Podaje kąt obrotu względem rodzica.
     * @return Kąt w radianach, z zakresu (-π, π].
     */
    float Rotation() const { return std::atan2(transform_.b, transform_.a); }

    /**
     * @brief Ustawia kąt obrotu względem rodzica, bez zmiany pozycji i skali.
     * @details Ustawienie bezwzględne odtwarza macierz z pozycji, kąta i skali, więc usuwa
     * pochylenie, które mogło powstać z ScaleBy() o różnych mnożnikach po obrocie.
     * @param radians Kąt w radianach.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Rotation(this Self& self, float radians) {
        Transformable& base = self;
        base.Compose(base.Position(), radians, base.Scale());
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
     * @brief Podaje skalę osi obiektu względem rodzica.
     * @details Ujemna składowa Y oznacza odbicie lustrzane.
     * @return Mnożniki skali osi X i Y.
     */
    Vector2 Scale() const {
        const float x = std::sqrt(transform_.a * transform_.a + transform_.b * transform_.b);
        if (x == 0.0f) return {0.0f, 0.0f};
        return {x, (transform_.a * transform_.d - transform_.b * transform_.c) / x};
    }

    /**
     * @brief Ustawia skalę osi obiektu, bez zmiany pozycji i obrotu.
     * @details Skala działa wzdłuż osi obiektu (już obróconych) i zastępuje bieżącą, więc
     * `Scale({2, 2})` wywołane dwa razy daje skalę 2. Zmianę o mnożnik robi ScaleBy().
     * @param factors Mnożniki skali osi X i Y.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Scale(this Self& self, Vector2 factors) {
        Transformable& base = self;
        base.Compose(base.Position(), base.Rotation(), factors);
        return self;
    }

    /**
     * @brief Mnoży skalę obiektu i jego dzieci względem punktu w układzie rodzica.
     * @param factors Mnożniki skali osi X i Y (układu rodzica).
     * @param pivot Środek skalowania w układzie rodzica.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& ScaleBy(this Self& self, Vector2 factors, Vector2 pivot) {
        self.transform_ = AffineMatrix2D::Translation(pivot) * AffineMatrix2D::Scaling(factors) *
                          AffineMatrix2D::Translation({-pivot.x, -pivot.y}) * self.transform_;
        return self;
    }

    /**
     * @brief Mnoży skalę obiektu względem jego własnej pozycji.
     * @param factors Mnożniki skali osi X i Y (układu rodzica).
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& ScaleBy(this Self& self, Vector2 factors) {
        return self.ScaleBy(factors, self.Position());
    }

  protected:
    AffineMatrix2D transform_; /**< Lokalna macierz obiektu. */

  private:
    /** @brief Składa macierz z pozycji, kąta i skali (kolejność: skala, obrót, przesunięcie). */
    void Compose(Vector2 position, float radians, Vector2 scale) {
        transform_ = AffineMatrix2D::Translation(position) * AffineMatrix2D::Rotation(radians) *
                     AffineMatrix2D::Scaling(scale);
    }
};
