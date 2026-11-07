/**
 * @file Entity.hpp
 * @brief Abstrakcyjna baza obiektów sceny: drzewo, współrzędne i przebieg klatki.
 */
#pragma once

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <raycade/entity/Contextable.hpp>
#include <raycade/entity/Drawable.hpp>
#include <raycade/entity/Focusable.hpp>
#include <raycade/entity/Transformable.hpp>
#include <raycade/entity/Updatable.hpp>

class Scene;
class SceneCamera;
class Entity;

/**
 * @brief Powiązania obiektu ze sceną, wspólne dla wszystkich jej obiektów.
 * @details Scena ma jeden taki zestaw, a każdy obiekt - tylko wskaźnik do niego (zamiast
 * osobnych wskaźników na scenę, kontekst, kamerę i fokus).
 */
struct SceneLinks {
    Scene* scene = nullptr;               /**< Scena. */
    const Contextable* context = nullptr; /**< Scena jako poziom kontekstu. */
    const SceneCamera* camera = nullptr;  /**< Kamera sceny. */
    FocusState* focus = nullptr;          /**< Fokus klawiatury sceny. */
};

/**
 * @brief Wynik trafiania myszą w jednej klatce, liczony przez scenę.
 * @details Patrz @ref entity_trafienia.
 */
struct PointerFrame {
    const Entity* target = nullptr;   /**< Najwyżej narysowany obiekt pod kursorem. */
    const Entity* press = nullptr;    /**< Odbiorca OnClick. */
    const Entity* release = nullptr;  /**< Odbiorca OnMouseUp. */
    const Entity* scroll = nullptr;   /**< Odbiorca OnScroll. */
    const Entity* drag = nullptr;     /**< Obiekt, który zaczyna przeciąganie. */
    const Focusable* focus = nullptr; /**< Obiekt, który dostaje fokus po kliknięciu. */
    const Entity* focused = nullptr;  /**< Obiekt z fokusem (do przekazania mu klawiatury). */
    bool pressed = false;             /**< Czy w tej klatce wciśnięto lewy przycisk myszy. */
};

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
 * - OnMouseUp - puszczenie lewego przycisku nad obiektem,
 * - OnScroll - kółko myszy nad obiektem,
 * - OnDragStart / OnDrag / OnDragEnd - przeciąganie lewym przyciskiem,
 * - OnKeyPressed / OnKeyReleased - naciśnięcie / puszczenie konkretnego klawisza.
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
 * Update() nie jest wirtualna, a Render() jest `final` - zdarzenia, dzieci i macierz obsługuje
 * zawsze Entity.
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
 *
 * @section entity_trafienia Trafianie myszą
 * Scena raz na klatkę zbiera obiekty pod kursorem (te, których Contains() zwraca true)
 * w kolejności od najwyżej narysowanego: obiekty ekranu (ScreenSpace()) nad obiektami świata,
 * wyższa warstwa nad niższą, dziecko nad rodzicem, a w tej samej warstwie - później dodany
 * nad wcześniejszym. Dla obiektów świata kursor jest najpierw przeliczany przez kamerę sceny.
 * - IsHovered(), OnMouseEnter i OnMouseLeave dotyczą najwyższego trafionego obiektu i jego
 *   przodków (jak :hover w CSS) - obiekt zasłonięty przez inny nie jest najechany.
 * - OnClick, OnMouseUp, OnScroll i przeciąganie dostaje jeden obiekt: pierwszy z takim
 *   zdarzeniem, szukając od najwyższego trafionego w górę jego przodków. Obiekt bez tego
 *   zdarzenia (np. ozdobny kontur albo część gracza) przepuszcza je do przodka, a gdy żaden
 *   przodek go nie obsługuje - do obiektów pod spodem.
 *
 * Żeby obiekt blokował kliknięcia w to, co pod nim (np. okno dialogowe), wystarczy dać mu
 * puste zdarzenie: `.OnClick([](auto&) {})`.
 *
 * @section entity_fokus Fokus klawiatury
 * Fokus mają tylko obiekty z cechą Focusable (np. ui::TextField) - patrz jej opis.
 *
 * @section entity_uklady Układy współrzędnych i kamera
 * - lokalny - względem pozycji obiektu (w nim rysuje Draw()),
 * - sceny (świata) - po złożeniu transformacji wszystkich przodków; w nim działają
 *   WorldPosition(), ToLocal(), Contains() i World(),
 * - ekranu (okna) - po przejściu przez kamerę sceny (SceneCamera); w nim jest kursor.
 *   Przeliczają do niego ScreenPosition() i ScreenToLocal().
 *
 * Obiekt ekranu (ScreenSpace(), np. HUD) pomija kamerę: dla niego układ sceny i ekranu to to
 * samo.
 *
 * @section entity_kontekst Kontekst (jak setContext/getContext w Svelte)
 * ProvideContext<T>() udostępnia wartość całemu poddrzewu obiektu, a Context<T>() w potomku
 * zwraca najbliższą wartość typu T: z samego obiektu, jego przodków, sceny
 * (Scene::ProvideContext()) albo silnika (Engine::ProvideContext()). Kluczem jest typ.
 * @code{.cpp}
 * struct Theme { Color accent = RED; };
 *
 * scene.Spawn<shapes::Group>()
 *     .ProvideContext<Theme>(Theme{GOLD})                 // całe menu w złotym kolorze
 *     .Children([](shapes::Group& menu) { menu.Spawn<Button>(); });
 *
 * // w Button::OnMount() albo Button::Tick():
 * if (const Theme* theme = Context<Theme>()) Paint(theme->accent);
 * @endcode
 * W konstruktorze obiekt nie zna jeszcze rodzica ani sceny - kontekst odczytuje się
 * w OnMount() albo później (Tick(), Draw(), zdarzenia).
 */
class Entity : public Updatable, public Drawable, public Transformable, public Contextable {
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
        // dziecko obiektu już podpiętego do sceny podpinamy od razu (wywoła jego OnMount());
        // dzieci tworzone w konstruktorze zostaną podpięte razem z rodzicem
        if (links_) ref.Attach(*links_);
        return ref;
    }

    /**
     * @brief Usuwa obiekt razem z dziećmi ze sceny.
     * @details Obiekt jest tylko oznaczany, a usuwany przez scenę na końcu jej Update() -
     * dlatego Destroy() można bezpiecznie wywołać w dowolnym zdarzeniu, także na sobie
     * (`.OnClick([](auto& self) { self.Destroy(); })`) i w pętli po innych obiektach.
     * Oznaczony obiekt nie dostaje już zdarzeń ani trafień myszą. Kamera, która za nim
     * podążała, przestaje podążać, a fokus klawiatury jest zdejmowany.
     *
     * Po usunięciu wszystkie referencje i wskaźniki do obiektu (np. zapamiętane z Spawn<T>())
     * są nieważne - obiekt, do którego trzeba się później odwoływać, sprawdza się przez
     * IsDestroyed() przed końcem klatki albo nie trzyma się do niego referencji.
     * @return Ten sam obiekt (ważny do końca bieżącej klatki).
     */
    template <typename Self> Self& Destroy(this Self& self) {
        self.destroyed_ = true;
        return self;
    }

    /** @brief Sprawdza, czy obiekt oznaczono do usunięcia (Destroy()) w tej klatce. */
    bool IsDestroyed() const { return destroyed_; }

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
     * @details Wywoływana przez scenę. Logikę klasy pochodnej umieszcza się w Tick().
     * @param dt Czas od poprzedniej klatki w sekundach.
     * @param input Stan klawiatury i myszy.
     * @param pointer Trafienia myszą policzone przez scenę w tej klatce.
     */
    void Update(float dt, const Input& input, const PointerFrame& pointer);

    /**
     * @brief Nakłada lokalną macierz i rysuje zawartość obiektu (RenderContent()).
     * @details Metoda jest `final`: własny wygląd klasy pochodnej rysuje się w Draw(),
     * a zmiana sposobu rysowania dzieci - w RenderContent().
     * @param renderer Renderer prymitywów.
     */
    void Render(PrimitiveRenderer& renderer) const final;

    /**
     * @brief Oznacza obiekt jako obiekt ekranu (HUD, menu): bez kamery i nad światem.
     * @details Liczy się ustawienie obiektu najwyższego poziomu - dzieci je dziedziczą.
     * @param value Czy obiekt jest w układzie ekranu.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& ScreenSpace(this Self& self, bool value = true) {
        self.screenSpace_ = value;
        return self;
    }

    /** @brief Sprawdza, czy obiekt (a właściwie jego przodek najwyższego poziomu) jest w układzie
     * ekranu. */
    bool IsScreenSpace() const;

    /**
     * @brief Oblicza pozycję obiektu w układzie sceny (z uwzględnieniem rodziców, bez kamery).
     * @return Pozycja w świecie.
     */
    Vector2 WorldPosition() const { return World({0.0f, 0.0f}); }

    /**
     * @brief Oblicza pozycję obiektu w oknie (po przejściu przez kamerę sceny).
     * @return Pozycja na ekranie.
     */
    Vector2 ScreenPosition() const;

    /**
     * @brief Przelicza punkt okna (np. kursor) na układ lokalny obiektu, uwzględniając kamerę.
     * @param point Punkt w układzie okna.
     * @return Punkt w układzie lokalnym albo {0, 0}, gdy macierz jest nieodwracalna.
     */
    Vector2 ScreenToLocal(Vector2 point) const { return ToLocal(ScreenToScene(point)); }

    /**
     * @brief Przelicza punkt sceny na układ lokalny obiektu.
     * @param point Punkt w układzie sceny.
     * @return Punkt w układzie lokalnym albo {0, 0}, gdy macierz jest nieodwracalna.
     */
    Vector2 ToLocal(Vector2 point) const { return TryToLocal(point).value_or(Vector2{}); }

    /**
     * @brief Przelicza punkt sceny na lokalny, jeśli przekształcenie jest odwracalne.
     * @param point Punkt w układzie sceny.
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

    /**
     * @brief Pobiera scenę, do której należy obiekt.
     * @return Wskaźnik sceny albo nullptr, gdy obiekt nie jest jeszcze podpięty.
     */
    Scene* OwnerScene() const { return links_ ? links_->scene : nullptr; }

  protected:
    /**
     * @brief Następny poziom kontekstu: rodzic, a dla obiektu najwyższego poziomu - scena.
     * @details Obiekt jeszcze niepodpięty (np. w konstruktorze) widzi tylko rodziców i silnik.
     * @return Poziom wyżej w łańcuchu kontekstu.
     */
    const Contextable* ContextParent() const override;

    /**
     * @brief Przelicza ruch kursora na przesunięcie w układzie rodzica.
     * @details Dzięki temu `Translate(delta)` w OnDrag() przesuwa obiekt dokładnie za
     * kursorem także w obróconej lub przeskalowanej grupie i przy przybliżonej kamerze.
     */
    Vector2 DragDelta(Vector2 screenFrom, Vector2 screenTo) const override;

    /**
     * @brief Wywoływana raz, gdy obiekt zostaje podpięty do sceny.
     * @details W tym momencie znani są już rodzic, przodkowie i scena, więc to miejsce na
     * odczyt kontekstu przy starcie (odpowiednik getContext przy inicjalizacji komponentu
     * Svelte). Rodzic dostaje OnMount() przed swoimi dziećmi.
     */
    virtual void OnMount() {}

    /**
     * @brief Rysuje zawartość obiektu w jego układzie lokalnym (macierz jest już nałożona).
     * @details Domyślnie: własny kształt (Draw()), a nad nim dzieci według warstw. Klasa
     * pochodna może to zmienić, np. shapes::Surface rysuje dzieci do tekstury.
     * @param renderer Renderer prymitywów.
     */
    virtual void RenderContent(PrimitiveRenderer& renderer) const {
        Drawable::Render(renderer);
        RenderChildren(renderer);
    }

    /**
     * @brief Rysuje dzieci według warstw (w bieżącym celu i układzie).
     * @param renderer Renderer prymitywów.
     */
    void RenderChildren(PrimitiveRenderer& renderer) const;

    /**
     * @brief Czy dzieci mogą zostać trafione myszą w tym punkcie.
     * @details Domyślnie zawsze. Obiekt, który przycina rysunek dzieci (np. shapes::Surface),
     * zwraca false poza swoim obszarem - niewidocznych części nie da się kliknąć.
     * @param point Punkt w układzie sceny.
     * @return Czy sprawdzać trafienia w dzieci.
     */
    virtual bool ChildrenHittable([[maybe_unused]] Vector2 point) const { return true; }

    /**
     * @brief Przelicza punkt lokalny (względem pozycji obiektu) na punkt sceny.
     * @param local Punkt lokalny.
     * @return Punkt w układzie sceny.
     */
    Vector2 World(Vector2 local) const { return WorldTransform().Apply(local); }

    /** @brief Łączy własne przekształcenie z przekształceniami rodziców. */
    AffineMatrix2D WorldTransform() const {
        if (!parent_) return transform_;
        return parent_->WorldTransform() * transform_;
    }

    /**
     * @brief Przelicza listę wierzchołków lokalnych na układ sceny.
     * @details Wynik jest typu Vector2, bo służy do testów trafień funkcjami raylib
     * (CheckCollisionPointPoly() itd.), które przyjmują tablice Vector2.
     * @param local Wierzchołki lokalne.
     * @return Wierzchołki w układzie sceny.
     */
    std::vector<Vector2> WorldVertices(std::span<const Point2D> local) const;

    /**
     * @brief Sprawdza, czy punkt trafia w któreś z dzieci (rekurencyjnie).
     * @details Ułatwia Contains() obiektów złożonych z części: grupa reaguje na mysz tam,
     * gdzie widać jej dzieci.
     * @param point Punkt w układzie sceny.
     * @return true, gdy któreś dziecko (albo jego potomek) zawiera punkt.
     */
    bool AnyChildContains(Vector2 point) const;

  private:
    friend class Scene;

    /**
     * @brief Dopisuje obiekty pod punktem w kolejności od najwyżej narysowanego.
     * @details Kolejność odwrotna do Render(): najpierw dzieci (od najwyższej warstwy, w niej
     * od ostatnio dodanego), potem sam obiekt, bo dzieci są rysowane nad rodzicem.
     * @param point Punkt w układzie sceny.
     * @param hits Lista, do której dopisywane są trafienia.
     */
    void CollectHits(Vector2 point, std::vector<const Entity*>& hits) const;

    /**
     * @brief Porządkuje obiekty od najwyżej narysowanego (odwrotnie niż Render()).
     * @param entities Obiekty jednego poziomu drzewa.
     * @return Wskaźniki od najwyższej warstwy, w warstwie od ostatnio dodanego.
     */
    static std::vector<const Entity*> TopFirst(
        const std::vector<std::unique_ptr<Entity>>& entities
    );

    /**
     * @brief Wybiera odbiorcę zdarzenia myszy (patrz @ref entity_trafienia).
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

    /**
     * @brief Liczy trafienia myszą dla jednej klatki.
     * @details Obiekty ekranu są sprawdzane pierwsze (leżą nad światem) z kursorem w układzie
     * okna, a obiekty świata - z kursorem przeliczonym przez kamerę.
     * @param roots Obiekty najwyższego poziomu sceny.
     * @param input Stan wejścia.
     * @param camera Kamera sceny.
     * @return Najwyższy trafiony obiekt i odbiorcy zdarzeń.
     */
    static PointerFrame Pointer(
        const std::vector<std::unique_ptr<Entity>>& roots,
        const Input& input,
        const SceneCamera& camera
    );

    /** @brief Sprawdza, czy obiekt to podany obiekt albo jego przodek. */
    bool IsSelfOrAncestorOf(const Entity* entity) const;

    /**
     * @brief Podpina obiekt i jego poddrzewo do sceny, wywołując OnMount() (raz na obiekt).
     * @param links Powiązania ze sceną (należą do sceny).
     */
    void Attach(const SceneLinks& links);

    Entity* parent_ = nullptr;          /**< Rodzic albo nullptr. */
    const SceneLinks* links_ = nullptr; /**< Powiązania ze sceną albo nullptr. */
    bool screenSpace_ = false;          /**< Czy obiekt jest w układzie ekranu. */
    bool destroyed_ = false;            /**< Czy obiekt czeka na usunięcie (Destroy()). */

    /** @brief Kamera, przez którą widać obiekt: nullptr dla obiektów ekranu i niepodpiętych. */
    const SceneCamera* ViewCamera() const;

    /** @brief Przelicza punkt okna na punkt sceny (przez kamerę, jeśli obiekt jej podlega). */
    Vector2 ScreenToScene(Vector2 point) const;

    bool mounted_ = false; /**< Czy OnMount() już się wykonało. */
    /** Dzieci obiektu. Pola są niszczone przed klasami bazowymi, więc dzieci znikają przed
     *  kontekstem z Contextable i mogą bezpiecznie trzymać do niego referencje. */
    std::vector<std::unique_ptr<Entity>> children_;
};
