/**
 * @file Contextable.hpp
 * @brief Kontekst - odpowiednik setContext/getContext ze Svelte: magazyn wartości
 * i klasa bazowa Contextable dla Entity, Scene i Engine.
 */
#pragma once

#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

/**
 * @brief Zbiór wartości kontekstu rozpoznawanych po typie.
 * @details Każdy typ może mieć w jednym magazynie najwyżej jedną wartość. Dwie wartości tego
 * samego typu rozróżnia się osobnymi typami, np. `struct Lives { int value; };`.
 *
 * Używany wewnętrznie przez Contextable.
 */
class ContextStore {
  public:
    /**
     * @brief Tworzy wartość typu T, którą magazyn przechowuje.
     * @details Jeśli magazyn ma już własną wartość typu T, nowa jest do niej przypisywana
     * w tym samym miejscu pamięci. Wskaźniki zapamiętane wcześniej przez Context() (np.
     * w OnMount()) pozostają więc ważne i od razu widzą nową wartość. Dotyczy to typów
     * z przypisaniem przenoszącym; pozostałe oraz wartości z Reference() są zastępowane.
     * @tparam T Typ wartości, będący jednocześnie kluczem.
     * @param args Argumenty konstruktora T.
     * @return Referencja do wartości.
     */
    template <typename T, typename... Args> T& Emplace(Args&&... args) {
        if constexpr (std::is_move_assignable_v<T>) {
            if (const auto it = values_.find(typeid(T)); it != values_.end() && it->second.owned) {
                T& existing = *static_cast<T*>(it->second.value.get());
                existing = T(std::forward<Args>(args)...);
                return existing;
            }
        }
        auto value = std::make_shared<T>(std::forward<Args>(args)...);
        T& ref = *value;
        values_[typeid(T)] = Entry{std::move(value), true};
        return ref;
    }

    /**
     * @brief Udostępnia istniejący obiekt bez przejmowania go na własność.
     * @details Obiekt musi żyć co najmniej tak długo jak właściciel magazynu.
     * @tparam T Typ obiektu, będący jednocześnie kluczem.
     * @param object Udostępniany obiekt.
     */
    template <typename T> void Reference(T& object) {
        values_[typeid(T)] =
            Entry{std::shared_ptr<void>(static_cast<void*>(&object), [](void*) {}), false};
    }

    /**
     * @brief Szuka wartości typu T w tym magazynie.
     * @return Wskaźnik wartości albo nullptr.
     */
    template <typename T> T* Find() const {
        const auto it = values_.find(typeid(T));
        return it == values_.end() ? nullptr : static_cast<T*>(it->second.value.get());
    }

    /** @brief Usuwa wszystkie wartości. */
    void Clear() { values_.clear(); }

  private:
    /** @brief Wartość w magazynie. */
    struct Entry {
        std::shared_ptr<void> value; /**< Wartość; shared_ptr<void> pamięta destruktor T. */
        bool owned = false;          /**< Czy magazyn jest właścicielem (Emplace()). */
    };

    std::unordered_map<std::type_index, Entry> values_; /**< Wartości według typu. */
};

class Contextable;

/**
 * @brief Korzeń łańcucha kontekstu - silnik (Engine::Get()).
 * @details Zdefiniowane w Engine.cpp, żeby Entity i Scene nie musiały dołączać Engine.hpp.
 * @return Wskaźnik silnika jako Contextable.
 */
Contextable* ContextRoot();

/**
 * @brief Klasa bazowa obiektów udostępniających kontekst: Entity, Scene i Engine.
 * @details Każdy obiekt ma własny magazyn (tworzony dopiero przy pierwszym ProvideContext()) i
 * wskazuje następny poziom przez ContextParent(). Context<T>() idzie tym łańcuchem i zwraca
 * wartość najbliższą obiektowi, więc niższy poziom przesłania wyższy:
 *
 *     Entity → rodzic → ... → Scene → Engine
 *
 * - ProvideContext<T>(args...) - tworzy nową wartość, którą obiekt przechowuje,
 * - SetContext(object) - udostępnia istniejący obiekt przez referencję (jak setContext
 *   w Svelte),
 * - Context<T>() - zwraca najbliższą wartość albo nullptr (jak getContext w Svelte).
 *
 * Settery używają jawnego parametru obiektu (C++23), więc zwracają konkretny typ i można je
 * łączyć w łańcuch: `scene.Spawn<Menu>().ProvideContext<Theme>().Position({0, 0})`.
 */
class Contextable {
  public:
    virtual ~Contextable() = default;

    /**
     * @brief Udostępnia temu obiektowi i wszystkiemu poniżej nową wartość typu T.
     * @details Wartość należy do obiektu i znika razem z nim. Ponowne wywołanie dla tego
     * samego typu przypisuje nową wartość w tym samym miejscu (patrz ContextStore::Emplace()).
     * @tparam T Typ wartości, będący jednocześnie kluczem.
     * @param args Argumenty konstruktora T.
     * @return Ten sam obiekt.
     */
    template <typename T, typename Self, typename... Args>
    Self& ProvideContext(this Self& self, Args&&... args) {
        self.OwnContext().template Emplace<T>(std::forward<Args>(args)...);
        return self;
    }

    /**
     * @brief Udostępnia istniejący obiekt bez kopiowania i przejmowania go na własność.
     * @details Obiekt musi żyć co najmniej tak długo jak ten obiekt, np. gracz udostępniony
     * przez scenę, w której jest.
     * @param object Udostępniany obiekt; jego typ jest kluczem.
     * @return Ten sam obiekt.
     */
    template <typename Self, typename T> Self& SetContext(this Self& self, T& object) {
        self.OwnContext().Reference(object);
        return self;
    }

    /**
     * @brief Szuka najbliższej wartości typu T: w tym obiekcie, a potem coraz wyżej.
     * @tparam T Szukany typ.
     * @return Wskaźnik wartości albo nullptr, gdy nikt jej nie udostępnił.
     */
    template <typename T> T* Context() const {
        for (const Contextable* level = this; level; level = level->ContextParent())
            if (level->context_)
                if (T* value = level->context_->template Find<T>()) return value;
        return nullptr;
    }

  protected:
    /**
     * @brief Następny poziom wyszukiwania kontekstu.
     * @return Wskaźnik poziomu wyżej albo nullptr dla korzenia (silnika).
     */
    virtual const Contextable* ContextParent() const { return nullptr; }

    /** @brief Usuwa wszystkie wartości udostępnione przez ten obiekt. */
    void ClearContext() { context_.reset(); }

  private:
    /** @brief Zwraca własny magazyn, tworząc go przy pierwszym użyciu. */
    ContextStore& OwnContext() {
        if (!context_) context_ = std::make_unique<ContextStore>();
        return *context_;
    }

    /** Magazyn tworzony dopiero przy ProvideContext() - obiekty bez kontekstu nic nie kosztują. */
    std::unique_ptr<ContextStore> context_;
};
