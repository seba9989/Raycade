/**
 * @file Scene.hpp
 * @brief Klasa Scene - zbiór obiektów i logiki jednego ekranu.
 */
#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "../render/PrimitiveRenderer.hpp"
#include "Entity.hpp"
#include "Input.hpp"

/**
 * @brief Zbiór obiektów i funkcji logicznych jednego ekranu silnika.
 * @details Silnik aktualizuje i rysuje wyłącznie aktywną scenę. Obiekty dodane
 * przez Spawn() są korzeniami drzew i mogą tworzyć własne dzieci.
 */
class Scene {
  public:
    /**
     * @brief Dodaje obiekt do sceny.
     * @tparam T Kształt z przestrzeni nazw @ref shapes albo własna klasa dziedzicząca
     * po Entity, np. `Spawn<shapes::Rect>()`.
     * @return Referencja do nowego obiektu, ważna przez cały czas życia sceny.
     */
    template <typename T> T& Spawn() {
        entities_.push_back(std::make_unique<T>());
        return static_cast<T&>(*entities_.back());
    }

    /**
     * @brief Ustawia logikę sceny wykonywaną co klatkę.
     * @param update Funkcja otrzymująca czas dt w sekundach.
     * @return Ta sama scena.
     */
    Scene& OnUpdate(std::function<void(float dt)> update) {
        update_ = std::move(update);
        return *this;
    }

    /**
     * @brief Ustawia dodatkowe rysowanie wykonywane po obiektach sceny.
     * @param draw Funkcja otrzymująca renderer.
     * @return Ta sama scena.
     */
    Scene& OnDraw(std::function<void(PrimitiveRenderer&)> draw) {
        draw_ = std::move(draw);
        return *this;
    }

    /**
     * @brief Aktualizuje logikę sceny i jej obiekty.
     * @param dt Czas od poprzedniej klatki w sekundach.
     * @param input Stan wejścia.
     */
    void Update(float dt, const Input& input) {
        if (update_) update_(dt);
        // pętla po indeksie: zdarzenie obiektu może wywołać Spawn() i powiększyć wektor
        for (std::size_t i = 0; i < entities_.size(); ++i)
            entities_[i]->Update(dt, input);
    }

    /**
     * @brief Rysuje drzewa obiektów według warstw, a następnie wywołuje OnDraw().
     * @details Kolejność: od najniższej warstwy, w tej samej warstwie - według dodania.
     * @param renderer Renderer prymitywów.
     */
    void Draw(PrimitiveRenderer& renderer) const {
        std::vector<const Drawable*> order;
        order.reserve(entities_.size());
        for (const auto& entity : entities_)
            order.push_back(entity.get());
        for (const Drawable* entity : Drawable::ByLayer(std::move(order)))
            entity->Render(renderer);
        if (draw_) draw_(renderer.Reset().TargetScreen());
    }

  private:
    std::vector<std::unique_ptr<Entity>> entities_; /**< Obiekty najwyższego poziomu. */
    std::function<void(float)> update_;             /**< Logika sceny co klatkę. */
    std::function<void(PrimitiveRenderer&)> draw_;  /**< Dodatkowe rysowanie sceny. */
};
