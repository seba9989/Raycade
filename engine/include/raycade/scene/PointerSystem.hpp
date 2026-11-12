/**
 * @file PointerSystem.hpp
 * @brief Klasa PointerSystem - wybór obiektów sceny pod kursorem i odbiorców zdarzeń myszy.
 */
#pragma once

#include <memory>
#include <vector>

#include <raycade/entity/Entity.hpp>
#include <raycade/input/Input.hpp>
#include <raycade/scene/SceneCamera.hpp>

/**
 * @brief System sceny, który raz na klatkę ustala, co jest pod kursorem myszy.
 * @details Przechodzi po drzewie obiektów sceny i pyta je o obszar trafienia
 * (Entity::Contains()), a potem wybiera odbiorców zdarzeń OnClick, OnMouseUp, OnScroll
 * i przeciągania według reguł z @ref entity_trafienia. Wynik (PointerFrame) scena przekazuje
 * obiektom w Entity::Update().
 *
 * Obiekty same nie znają tego algorytmu - mają tylko Contains() i zdarzenia. Dzięki temu
 * zapytania obejmujące całą scenę (trafienia myszą, a w przyszłości kolizje) są w systemach
 * sceny, a nie w klasie Entity.
 */
class PointerSystem {
  public:
    /**
     * @brief Liczy trafienia myszą dla jednej klatki.
     * @details Obiekty ekranu są sprawdzane pierwsze (leżą nad światem) z kursorem w układzie
     * okna, a obiekty świata - z kursorem przeliczonym przez kamerę.
     * @param roots Obiekty najwyższego poziomu sceny.
     * @param input Stan wejścia.
     * @param camera Kamera sceny.
     * @return Najwyższy trafiony obiekt i odbiorcy zdarzeń.
     */
    PointerFrame Pick(
        const std::vector<std::unique_ptr<Entity>>& roots,
        const Input& input,
        const SceneCamera& camera
    ) const;

    /**
     * @brief Zbiera obiekty pod punktem okna, od najwyżej narysowanego.
     * @param roots Obiekty najwyższego poziomu sceny.
     * @param screen Punkt w układzie okna (np. kursor).
     * @param camera Kamera sceny - przez nią przeliczany jest punkt dla obiektów świata.
     * @return Trafione obiekty: najpierw obiekty ekranu, potem świata.
     */
    std::vector<const Entity*> HitsAt(
        const std::vector<std::unique_ptr<Entity>>& roots,
        Vector2 screen,
        const SceneCamera& camera
    ) const;

  private:
    /**
     * @brief Dopisuje obiekt i jego poddrzewo pod punktem w kolejności od najwyżej narysowanego.
     * @details Kolejność odwrotna do rysowania: najpierw dzieci (od najwyższej warstwy, w niej
     * od ostatnio dodanego), potem sam obiekt, bo dzieci są rysowane nad rodzicem.
     * @param entity Obiekt, od którego zaczyna się sprawdzanie.
     * @param point Punkt w układzie sceny.
     * @param hits Lista, do której dopisywane są trafienia.
     */
    static void CollectHits(const Entity& entity, Vector2 point, std::vector<const Entity*>& hits);

    /**
     * @brief Porządkuje obiekty od najwyżej narysowanego (odwrotnie niż rysowanie).
     * @param entities Obiekty jednego poziomu drzewa.
     * @return Wskaźniki od najwyższej warstwy, w warstwie od ostatnio dodanego.
     */
    static std::vector<const Entity*> TopFirst(
        const std::vector<std::unique_ptr<Entity>>& entities
    );

    /**
     * @brief Wybiera odbiorcę zdarzenia myszy (patrz @ref entity_trafienia).
     * @details Pierwszy obiekt obsługujący zdarzenie, szukając od najwyżej trafionego w górę
     * jego przodków, a potem w kolejnych trafieniach.
     * @param hits Trafienia od najwyżej narysowanego.
     * @param handles Czy obiekt obsługuje zdarzenie.
     * @return Odbiorca albo nullptr.
     */
    template <typename Predicate>
    static const Entity* Receiver(const std::vector<const Entity*>& hits, Predicate handles) {
        for (const Entity* hit : hits)
            for (const Entity* entity = hit; entity; entity = entity->parent_)
                if (handles(*entity)) return entity;
        return nullptr;
    }
};
