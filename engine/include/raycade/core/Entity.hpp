/**
 * @file Entity.hpp
 * @brief Abstrakcyjna baza obiektów sceny: drzewo, współrzędne i przebieg klatki.
 */
#pragma once

#include <raylib.h>
#include <rlgl.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "Drawable.hpp"
#include "Transformable.hpp"
#include "Updatable.hpp"

/**
 * @brief Abstrakcyjna baza wszystkich obiektów sceny.
 * @details Obiekty tworzą drzewo: każdy może mieć dzieci (Spawn<T>()), a dzieci kolejne
 * dzieci. Pozycja dziecka jest względna wobec rodzica, więc przesunięcie rodzica przesuwa
 * całe poddrzewo. Dzieci są zawsze rysowane nad swoim rodzicem.
 * Updatable obsługuje własne zdarzenia, Drawable rysuje własny kształt i sortuje
 * warstwy, a Transformable przechowuje i zmienia macierz. Entity aktualizuje i
 * rysuje dzieci oraz nakłada macierz podczas rysowania.
 *
 * Obsługiwane zdarzenia (sprawdzane w Update()):
 * - OnUpdate - co klatkę,
 * - OnClick - kliknięcie lewym przyciskiem myszy na obiekcie,
 * - OnMouseEnter / OnMouseLeave - kursor wszedł na obiekt / zszedł z niego,
 * - OnKeyPressed - naciśnięcie konkretnego klawisza.
 *
 * @section entity_lancuch Łańcuchowanie
 * Settery i zdarzenia z klas bazowych (Position(), Rotate(), Paint(), OnClick() itd.) używają
 * jawnego parametru obiektu z C++23 (`this Self& self`). Zwracają więc konkretny typ, na którym
 * je wywołano, a callbacki zdarzeń dostają ten typ jako argument:
 * @code{.cpp}
 * scene.Spawn<shapes::Circle>()
 *     .Position({400, 300})
 *     .Rotate(PI / 4)                   // nadal shapes::Circle&
 *     .Radius(30)                       // setter samego okręgu
 *     .OnClick([](shapes::Circle& self) { self.Radius(50); });
 * @endcode
 *
 * @section entity_wlasne Własny kształt
 * Wystarczy dziedziczyć po Entity i nadpisać Draw(). Dla reakcji na mysz należy
 * nadpisać także Contains(); domyślnie obiekt nie ma obszaru trafienia. Umieszczenie klasy
 * w przestrzeni nazw `shapes` sprawia, że edytor podpowie ją razem z wbudowanymi.
 * @code{.cpp}
 * namespace shapes {
 * class Star : public Entity {
 *   public:
 *     bool Contains(Vector2 point) const override;
 *   protected:
 *     void Draw(PrimitiveRenderer& renderer) const override;
 * };
 * } // namespace shapes
 *
 * scene.Spawn<shapes::Star>().Position({100, 100}).Paint(GOLD);
 * @endcode
 *
 * @section entity_komponent Własny komponent wielokrotnego użytku
 * Klasa pochodna może:
 * - zbudować swoje części jako dzieci w konstruktorze (Spawn<T>()),
 * - mieć logikę w Tick() - scena nadal może dołożyć własną przez OnUpdate(),
 * - mieć własne settery z `this Self& self`, żeby łańcuch działał także w klasach pochodnych,
 * - reagować na mysz całą grupą dzięki AnyChildContains().
 *
 * Update() i Render() są `final` - zdarzenia, dzieci i macierz obsługuje zawsze Entity.
 * @code{.cpp}
 * class Player : public shapes::Group {
 *   public:
 *     Player() { Spawn<shapes::Rect>().Position({-15, -20}).Size({30, 40}).Fill(); }
 *     template <typename Self> Self& Speed(this Self& self, float speed) {
 *         self.speed_ = speed;
 *         return self;
 *     }
 *     bool Contains(Vector2 point) const override { return AnyChildContains(point); }
 *   protected:
 *     void Tick(float dt, const Input& input) override {
 *         if (input.KeyDown(KEY_D)) Translate({speed_ * dt, 0});
 *     }
 *   private:
 *     float speed_ = 200.0f;
 * };
 *
 * scene.Spawn<Player>().Position({400, 300}).Speed(300).OnClick([](Player& self) { ... });
 * @endcode
 */
class Entity : public Updatable, public Drawable, public Transformable {
  public:
    /** @brief Wirtualny destruktor umożliwiający usuwanie obiektów przez wskaźnik Entity. */
    virtual ~Entity() = default;

    /**
     * @brief Tworzy dziecko wybranego typu.
     * @tparam T Kształt z przestrzeni nazw @ref shapes albo własna klasa dziedzicząca
     * po Entity, np. `Spawn<shapes::Circle>()`.
     * @return Referencja do nowego dziecka, do dalszej konfiguracji.
     */
    template <typename T> T& Spawn() {
        auto child = std::make_unique<T>();
        T& ref = *child;
        child->parent_ = this;
        children_.push_back(std::move(child));
        return ref;
    }

    /**
     * @brief Wykonuje konfigurację dzieci i pozwala kontynuować łańcuch.
     * @param setup Funkcja otrzymująca ten obiekt jako jego konkretny typ
     * (dzieci tworzy się w niej przez Spawn<T>()).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Children(this Self& self, const Callback<Self>& setup) {
        setup(self);
        return self;
    }

    /**
     * @brief Obsługuje własne zdarzenia, a następnie aktualizuje dzieci.
     * @details Metoda jest `final`: logikę klasy pochodnej umieszcza się w Tick().
     * @param dt Czas od poprzedniej klatki w sekundach.
     * @param input Stan klawiatury i myszy.
     */
    void Update(float dt, const Input& input) final {
        Updatable::Update(dt, input);
        // Callback może dodać dziecko, dlatego rozmiar sprawdzamy w każdej iteracji.
        for (std::size_t i = 0; i < children_.size(); ++i)
            children_[i]->Update(dt, input);
    }

    /**
     * @brief Nakłada lokalną macierz, rysuje własny kształt, a potem dzieci według warstw.
     * @details Metoda jest `final`: własny wygląd klasy pochodnej rysuje się w Draw().
     * @param renderer Renderer prymitywów.
     */
    void Render(PrimitiveRenderer& renderer) const final {
        const auto matrix = transform_.RlglMatrix();
        rlPushMatrix();
        rlMultMatrixf(matrix.data());
        Drawable::Render(renderer);
        std::vector<const Drawable*> order;
        order.reserve(children_.size());
        for (const auto& child : children_)
            order.push_back(child.get());
        for (const Drawable* child : Drawable::ByLayer(std::move(order)))
            child->Render(renderer);
        rlPopMatrix();
    }

    /**
     * @brief Oblicza pozycję obiektu w układzie ekranu (z uwzględnieniem rodziców).
     * @return Pozycja na ekranie.
     */
    Vector2 WorldPosition() const { return World({0.0f, 0.0f}); }

    /**
     * @brief Przelicza punkt ekranowy (np. kursor) na układ lokalny obiektu.
     * @param point Punkt w układzie ekranu.
     * @return Punkt w układzie lokalnym albo {0, 0}, gdy macierz jest nieodwracalna.
     */
    Vector2 ToLocal(Vector2 point) const { return TryToLocal(point).value_or(Vector2{}); }

    /**
     * @brief Przelicza punkt ekranowy na lokalny, jeśli przekształcenie jest odwracalne.
     * @param point Punkt w układzie ekranu.
     * @return Punkt lokalny albo brak wyniku, np. po skalowaniu przez zero.
     */
    std::optional<Vector2> TryToLocal(Vector2 point) const {
        return WorldTransform().InverseApply(point);
    }

    /**
     * @brief Pobiera rodzica obiektu.
     * @return Wskaźnik rodzica albo nullptr dla obiektu na najwyższym poziomie sceny.
     */
    Entity* Parent() const { return parent_; }

  protected:
    /**
     * @brief Przelicza punkt lokalny (względem pozycji obiektu) na punkt ekranowy.
     * @param local Punkt lokalny.
     * @return Punkt w układzie ekranu.
     */
    Vector2 World(Vector2 local) const { return WorldTransform().Apply(local); }

    /** @brief Łączy własne przekształcenie z przekształceniami rodziców. */
    AffineMatrix2D WorldTransform() const {
        if (!parent_) return transform_;
        return parent_->WorldTransform() * transform_;
    }

    /**
     * @brief Przelicza listę wierzchołków lokalnych na ekranowe.
     * @tparam Vertex Typ wierzchołka (Vector2 albo Point2D).
     * @param local Wierzchołki lokalne.
     * @return Wierzchołki w układzie ekranu.
     */
    template <typename Vertex>
    std::vector<Vertex> WorldVertices(const std::vector<Vertex>& local) const {
        std::vector<Vertex> world;
        world.reserve(local.size());
        for (const Vertex& vertex : local)
            world.push_back(Vertex{World(vertex)});
        return world;
    }

    /**
     * @brief Sprawdza, czy punkt trafia w któreś z dzieci (rekurencyjnie).
     * @details Ułatwia Contains() obiektów złożonych z części: grupa reaguje na mysz tam,
     * gdzie widać jej dzieci.
     * @param point Punkt w układzie ekranu.
     * @return true, gdy któreś dziecko (albo jego potomek) zawiera punkt.
     */
    bool AnyChildContains(Vector2 point) const {
        for (const auto& child : children_)
            if (child->Contains(point) || child->AnyChildContains(point)) return true;
        return false;
    }

  private:
    Entity* parent_ = nullptr;                      /**< Rodzic albo nullptr. */
    std::vector<std::unique_ptr<Entity>> children_; /**< Dzieci obiektu. */
};
