/**
 * @file Engine.hpp
 * @brief Klasa Engine (okno, pętla gry, sceny, wejście) i makro SCENE.
 */
#pragma once

#include <raylib.h>

#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "../render/Bitmap.hpp"
#include "../render/PrimitiveRenderer.hpp"
#include "Input.hpp"
#include "Scene.hpp"
#include "Shapes.hpp" // IWYU pragma: export

/**
 * @brief Główny obiekt silnika zarządzający oknem, scenami, wejściem i pętlą gry.
 * @details Silnik jest singletonem: Engine::Get() zwraca zawsze tę samą instancję.
 * Metody konfiguracji zwracają Engine&, dlatego można je łączyć w łańcuch.
 * Ustawienia okna można zmieniać także po Init() - zmieniają wtedy okno na bieżąco.
 *
 * W każdej klatce Run() wykonuje kolejno: globalne OnUpdate(), aktualizację aktywnej
 * sceny, czyszczenie ekranu, rysowanie sceny, globalne OnDraw() i zamianę buforów.
 *
 * @code{.cpp}
 * Engine& engine = Engine::Get().Title("Gra").Window(800, 600).Fps(60).Init();
 * if (!engine.Ready()) return 1;
 *
 * engine.CreateScene("menu", [&](Scene& scene) { // pierwsza scena staje się aktywna
 *     scene.Spawn<shapes::Circle>().Position({400, 300}).Radius(20).Paint(ORANGE).Fill();
 * });
 *
 * return engine
 *     .OnUpdate([&](float) {
 *         if (engine.KeyPressed(KEY_ENTER)) engine.SwitchScene("menu");
 *     })
 *     .Run();
 * @endcode
 */
class Engine {
  public:
    /**
     * @brief Zwraca jedyną instancję silnika (singleton Meyersa).
     * @return Referencja do silnika, tworzonego przy pierwszym wywołaniu.
     */
    static Engine& Get() {
        static Engine instance;
        return instance;
    }

    /** @brief Kopiowanie silnika jest zabronione. */
    Engine(const Engine&) = delete;
    /** @brief Przypisanie kopii silnika jest zabronione. */
    Engine& operator=(const Engine&) = delete;

    /** @name Konfiguracja okna */
    /** @{ */

    /**
     * @brief Ustawia tytuł okna.
     * @param title Nowy tytuł.
     * @return Ten sam silnik.
     */
    Engine& Title(std::string title);

    /**
     * @brief Ustawia rozmiar okna i wyłącza tryb pełnoekranowy.
     * @param width Szerokość w pikselach.
     * @param height Wysokość w pikselach.
     * @return Ten sam silnik.
     */
    Engine& Window(int width, int height);

    /**
     * @brief Włącza lub wyłącza tryb pełnoekranowy.
     * @param value Czy włączyć ten tryb.
     * @return Ten sam silnik.
     */
    Engine& Fullscreen(bool value = true);

    /**
     * @brief Ustawia docelową liczbę klatek na sekundę.
     * @param fps Docelowa liczba klatek.
     * @return Ten sam silnik.
     */
    Engine& Fps(int fps);

    /**
     * @brief Włącza lub wyłącza synchronizację pionową (V-Sync).
     * @details Wielokrotne buforowanie: raylib (przez GLFW) zawsze tworzy okno z podwójnym
     * buforem ramki - rysujemy w tylnym buforze, a EndDrawing() zamienia go z przednim.
     * V-Sync synchronizuje tę zamianę z odświeżaniem monitora, co usuwa rozrywanie obrazu
     * (tearing). Podwójne buforowanie działa niezależnie od tego ustawienia.
     * @param value Czy włączyć V-Sync.
     * @return Ten sam silnik.
     */
    Engine& VSync(bool value = true);

    /**
     * @brief Ustawia kolor czyszczenia ekranu na początku każdej klatki.
     * @param color Kolor tła.
     * @return Ten sam silnik.
     */
    Engine& Background(Color color) {
        background_ = color;
        return *this;
    }

    /** @} */

    /** @name Konfiguracja wejścia i logów */
    /** @{ */

    /**
     * @brief Włącza lub wyłącza odczyt klawiatury.
     * @param value Czy włączyć klawiaturę.
     * @return Ten sam silnik.
     */
    Engine& Keyboard(bool value = true) {
        input_.Keyboard(value);
        return *this;
    }

    /**
     * @brief Włącza lub wyłącza odczyt myszy.
     * @details Wyłączenie myszy wyłącza też zdarzenia myszy obiektów sceny.
     * @param value Czy włączyć mysz.
     * @return Ten sam silnik.
     */
    Engine& Mouse(bool value = true) {
        input_.Mouse(value);
        return *this;
    }

    /**
     * @brief Ustawia ścieżkę pliku diagnostycznego.
     * @param path Ścieżka pliku; pusty napis wyłącza zapis do pliku.
     * @return Ten sam silnik.
     */
    Engine& LogFile(std::string path) {
        logFile_ = std::move(path);
        return *this;
    }

    /** @} */

    /** @name Logika globalna (niezależna od aktywnej sceny) */
    /** @{ */

    /**
     * @brief Ustawia globalną funkcję aktualizacji.
     * @param update Funkcja wywoływana co klatkę z czasem dt w sekundach.
     * @return Ten sam silnik.
     */
    Engine& OnUpdate(std::function<void(float dt)> update) {
        update_ = std::move(update);
        return *this;
    }

    /**
     * @brief Ustawia globalną funkcję rysowania (np. HUD widoczny w każdej scenie).
     * @param draw Funkcja wywoływana co klatkę po narysowaniu sceny.
     * @return Ten sam silnik.
     */
    Engine& OnDraw(std::function<void(PrimitiveRenderer&)> draw) {
        draw_ = std::move(draw);
        return *this;
    }

    /** @} */

    /** @name Cykl życia */
    /** @{ */

    /**
     * @brief Otwiera okno i inicjalizuje silnik, jeśli jeszcze nie jest gotowy.
     * @details Błąd utworzenia okna jest zapisywany w logu.
     * @return Ten sam silnik; wynik można sprawdzić przez Ready().
     */
    Engine& Init();

    /**
     * @brief Sprawdza wynik inicjalizacji.
     * @return true, gdy okno jest gotowe.
     */
    bool Ready() const { return initialized_; }

    /**
     * @brief Uruchamia pętlę gry aż do zamknięcia okna, a potem sprząta.
     * @return 0 po normalnym zakończeniu albo 1 po błędzie inicjalizacji (kod wyjścia dla main).
     */
    int Run();

    /**
     * @brief Zwalnia sceny, tekstury i okno.
     * @details Sceny są usuwane przed zamknięciem okna, bo tekstury bitmap wymagają
     * aktywnego kontekstu OpenGL. Ponowne wywołanie jest bezpieczne.
     */
    void Shutdown();

    /** @} */

    /**
     * @name Sceny
     * Aktualizowana i rysowana jest tylko aktywna scena; nieaktywne pozostają zamrożone.
     *
     * Sceny zarejestrowane (makro SCENE, RegisterScene()) są wczytywane leniwie: konfiguracja
     * sceny wykonuje się dopiero przy pierwszym wejściu na nią (scena startowa - na początku
     * Run(), pozostałe - przy pierwszym SwitchScene()). Raz wczytana scena zostaje w pamięci
     * do końca działania silnika i po powrocie zachowuje swój stan.
     */
    /** @{ */

    /**
     * @brief Tworzy scenę i natychmiast wykonuje jej konfigurację.
     * @param name Nazwa sceny.
     * @param setup Funkcja otrzymująca utworzoną scenę.
     * @return Ten sam silnik.
     */
    Engine& CreateScene(const std::string& name, const std::function<void(Scene&)>& setup);

    /**
     * @brief Tworzy scenę albo zwraca już istniejącą o tej nazwie.
     * @details Pierwsza utworzona scena staje się aktywna.
     * @param name Nazwa sceny.
     * @return Referencja do sceny.
     */
    Scene& CreateScene(const std::string& name);

    /**
     * @brief Rejestruje konfigurację sceny wczytywanej leniwie.
     * @details Scena powstaje dopiero przy pierwszym wejściu na nią, zawsze po Init(), więc
     * w konfiguracji można wczytywać tekstury. Zwykle wywoływane przez makro SCENE.
     * Ponowna rejestracja tej samej nazwy jest ignorowana i zapisywana w logu.
     * @param name Nazwa sceny.
     * @param setup Funkcja konfigurująca scenę.
     * @return Ten sam silnik.
     */
    Engine& RegisterScene(std::string name, std::function<void(Scene&)> setup);

    /**
     * @brief Wybiera scenę startową spośród zarejestrowanych scen.
     * @details Kolejność rejestracji scen z różnych plików jest nieokreślona, dlatego scenę
     * startową trzeba wskazać jawnie.
     * @param name Nazwa sceny.
     * @return Ten sam silnik.
     */
    Engine& StartScene(std::string name) {
        start_ = std::move(name);
        return *this;
    }

    /**
     * @brief Pobiera scenę o podanej nazwie, w razie potrzeby wczytując ją.
     * @details Wczytanie zarejestrowanej sceny wymaga otwartego okna (po Init()).
     * @param name Nazwa sceny.
     * @return Referencja do sceny.
     * @throw std::out_of_range Gdy scena nie istnieje ani nie jest zarejestrowana.
     */
    Scene& GetScene(const std::string& name);

    /**
     * @brief Sprawdza, czy scena została już wczytana.
     * @param name Nazwa sceny.
     * @return true, gdy scena istnieje w pamięci.
     */
    bool SceneLoaded(const std::string& name) const { return scenes_.contains(name); }

    /**
     * @brief Wczytuje zarejestrowaną scenę od razu, bez przełączania na nią.
     * @details Pozwala wczytać ciężką scenę z wyprzedzeniem (np. na ekranie ładowania), żeby
     * późniejsze SwitchScene() było natychmiastowe. Już wczytana scena pozostaje bez zmian -
     * do ponownego wczytania służy ReloadScene(). Wymaga otwartego okna (po Init()).
     * @param name Nazwa zarejestrowanej sceny.
     * @return Ten sam silnik.
     */
    Engine& LoadScene(const std::string& name);

    /**
     * @brief Zleca zwolnienie sceny (obiektów i ich tekstur) na koniec bieżącej klatki.
     * @details Zwolnienie jest odroczone, bo scena może zlecić je z własnego zdarzenia, w trakcie
     * pętli po swoich obiektach. Następne wejście na scenę wczyta ją od nowa. Zwolnienie
     * aktywnej sceny bez przełączenia na inną oznacza jej restart. Sceny utworzonej przez
     * CreateScene() (bez rejestracji) nie da się wczytać ponownie, dlatego aktywnej takiej
     * sceny nie można zwolnić.
     * @warning Po zwolnieniu referencje do sceny i jej obiektów (np. z GetScene()) są
     * nieważne.
     * @param name Nazwa sceny.
     * @return Ten sam silnik.
     */
    Engine& UnloadScene(const std::string& name);

    /**
     * @brief Zleca zwolnienie i ponowne wczytanie sceny na koniec bieżącej klatki.
     * @details Konfiguracja sceny wykonuje się od nowa, więc obiekty wracają do stanu
     * początkowego (np. restart poziomu). Działa także dla sceny aktywnej.
     * @param name Nazwa zarejestrowanej sceny.
     * @return Ten sam silnik.
     */
    Engine& ReloadScene(const std::string& name);

    /**
     * @brief Zleca zmianę aktywnej sceny na koniec bieżącej klatki.
     * @details Niewczytana scena jest wtedy wczytywana. Nieznana nazwa jest zapisywana jako
     * błąd w logu.
     * @param name Nazwa sceny utworzonej albo zarejestrowanej.
     * @return Ten sam silnik.
     */
    Engine& SwitchScene(const std::string& name);

    /**
     * @brief Podaje nazwę aktywnej sceny.
     * @return Referencja do nazwy.
     */
    const std::string& ActiveScene() const { return active_; }

    /** @} */

    /** @name Klawiatura i mysz */
    /** @{ */

    /**
     * @brief Sprawdza, czy klawisz jest aktualnie wciśnięty.
     * @param key Kod klawisza raylib.
     * @return Stan klawisza.
     */
    bool KeyDown(int key) const { return input_.KeyDown(key); }

    /**
     * @brief Sprawdza, czy klawisz naciśnięto w bieżącej klatce.
     * @param key Kod klawisza raylib.
     * @return Stan naciśnięcia.
     */
    bool KeyPressed(int key) const { return input_.KeyPressed(key); }

    /**
     * @brief Sprawdza, czy przycisk myszy jest wciśnięty.
     * @param button Kod przycisku raylib.
     * @return Stan przycisku.
     */
    bool MouseDown(int button) const { return input_.MouseDown(button); }

    /**
     * @brief Sprawdza, czy przycisk myszy naciśnięto w bieżącej klatce.
     * @param button Kod przycisku raylib.
     * @return Stan naciśnięcia.
     */
    bool MousePressed(int button) const { return input_.MousePressed(button); }

    /**
     * @brief Pobiera pozycję kursora w oknie.
     * @return Współrzędne kursora albo {0, 0}, gdy mysz jest wyłączona.
     */
    Vector2 MousePosition() const { return input_.MousePosition(); }

    /** @} */

    /** @name Czyszczenie ekranu i bitmap */
    /** @{ */

    /**
     * @brief Czyści aktualny cel rysowania wskazanym kolorem.
     * @param color Kolor czyszczenia.
     * @return Ten sam silnik.
     */
    Engine& Clear(Color color) {
        ::ClearBackground(color);
        return *this;
    }

    /**
     * @brief Czyści teksturę renderowaną.
     * @param target Tekstura docelowa.
     * @param color Kolor czyszczenia.
     * @return Ten sam silnik.
     */
    Engine& Clear(RenderTexture2D& target, Color color);

    /**
     * @brief Czyści bitmapę w pamięci RAM.
     * @param bitmap Bitmapa docelowa.
     * @param color Kolor czyszczenia.
     * @return Ten sam silnik.
     */
    Engine& Clear(Bitmap& bitmap, Color color) {
        bitmap.Clear(color);
        return *this;
    }

    /** @} */

    /** @name Rysowanie i diagnostyka */
    /** @{ */

    /**
     * @brief Udostępnia renderer prymitywów silnika.
     * @return Referencja do renderera.
     */
    PrimitiveRenderer& Primitives() { return primitives_; }

    /**
     * @brief Zapisuje komunikat do stderr i opcjonalnego pliku logu.
     * @param message Treść komunikatu.
     * @return Ten sam silnik.
     */
    Engine& Log(const std::string& message);

    /** @} */

  private:
    /** @brief Prywatny konstruktor - instancję zwraca wyłącznie Get(). */
    Engine() = default;
    /** @brief Destruktor wywołuje Shutdown(). */
    ~Engine() { Shutdown(); }

    std::string title_ = "Gra";          /**< Tytuł okna. */
    int width_ = 800;                    /**< Szerokość okna w trybie okienkowym. */
    int height_ = 600;                   /**< Wysokość okna w trybie okienkowym. */
    bool fullscreen_ = false;            /**< Czy używać trybu pełnoekranowego. */
    bool vsync_ = false;                 /**< Czy włączyć synchronizację pionową. */
    int fps_ = 60;                       /**< Docelowa liczba klatek na sekundę. */
    Color background_ = BLACK;           /**< Kolor czyszczenia ekranu w każdej klatce. */
    Input input_;                        /**< Stan i ustawienia klawiatury oraz myszy. */
    std::string logFile_ = "engine.log"; /**< Ścieżka pliku logu; pusta wyłącza zapis. */

    std::function<void(float)> update_;            /**< Globalna logika co klatkę. */
    std::function<void(PrimitiveRenderer&)> draw_; /**< Globalne rysowanie po scenie. */

    std::ofstream log_;        /**< Otwarty plik logu. */
    bool initialized_ = false; /**< Czy okno zostało utworzone. */

    std::map<std::string, Scene> scenes_; /**< Utworzone sceny według nazw. */
    std::string active_;                  /**< Nazwa aktywnej sceny. */
    std::string pending_; /**< Scena, na którą przełączamy po zakończeniu klatki. */
    std::string start_;   /**< Scena startowa wybrana przez StartScene(). */
    /** Konfiguracje scen zarejestrowanych makrem SCENE, wykonywane przy pierwszym wejściu. */
    std::map<std::string, std::function<void(Scene&)>> registered_;
    PrimitiveRenderer primitives_; /**< Renderer używany do rysowania scen. */
    /** Czas wczytywania scen w bieżącej klatce, odejmowany od dt następnej. */
    double loadSeconds_ = 0.0;
    std::set<std::string> unload_; /**< Sceny do zwolnienia na koniec klatki. */
    std::set<std::string> reload_; /**< Sceny do ponownego wczytania na koniec klatki. */

    /**
     * @brief Wykonuje odroczone przełączenie, zwolnienia i ponowne wczytania scen.
     */
    void ApplySceneChanges();

    /**
     * @brief Zwraca scenę, wczytując ją przy pierwszym użyciu.
     * @param name Nazwa sceny.
     * @return Wskaźnik sceny albo nullptr, gdy scena nie istnieje ani nie jest zarejestrowana.
     */
    Scene* Load(const std::string& name);

    /**
     * @brief Wybiera i wczytuje scenę startową na początku Run().
     */
    void LoadStartScene();
};

/**
 * @brief Pomocniczy rejestrator scen wykorzystywany przez makro SCENE.
 * @details Statyczny obiekt tej klasy powstaje przed main(), a jego konstruktor rejestruje
 * funkcję konfigurującą scenę w silniku.
 */
struct SceneRegistration {
    /**
     * @brief Zapamiętuje funkcję konfigurującą scenę.
     * @param name Nazwa sceny.
     * @param setup Funkcja konfigurująca.
     */
    SceneRegistration(const char* name, void (*setup)(Scene&)) {
        Engine::Get().RegisterScene(name, setup);
    }
};

/** @cond PRYWATNE */
#define SCENE_CONCAT_(a, b) a##b
#define SCENE_CONCAT(a, b) SCENE_CONCAT_(a, b)
#define SCENE_IMPL_(name, setup)                                                                   \
    static void setup(Scene& scene);                                                               \
    static const SceneRegistration SCENE_CONCAT(setup, _registration){name, setup};                \
    static void setup(Scene& scene)
/** @endcond */

/**
 * @def SCENE(name)
 * @brief Rejestruje scenę z bloku kodu wykonywanego po inicjalizacji okna.
 * @details Wewnątrz bloku dostępna jest zmienna `scene` (Scene&). Blok wykona się przy
 * pierwszym wejściu na scenę (wczytywanie leniwe), a scenę startową wybiera
 * Engine::StartScene(). Makro jest przeznaczone dla plików .cpp gry - nie należy go używać
 * w bibliotece statycznej, bo linker może pominąć plik, do którego nic się nie odwołuje.
 * @code{.cpp}
 * SCENE("menu") {
 *     scene.Spawn<shapes::Rect>().Position({300, 200}).Size({400, 300});
 * }
 * @endcode
 * @param name Nazwa sceny używana przez StartScene() i SwitchScene().
 */
#define SCENE(name) SCENE_IMPL_(name, SCENE_CONCAT(scene_setup_, __COUNTER__))
