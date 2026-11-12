/**
 * @file Engine.hpp
 * @brief Klasa Engine (okno, pętla gry, sceny, wejście).
 * @details Typy ustawień (LogLevel, Scaling) są w engine/EngineTypes.hpp, który dołącza ten
 * nagłówek. Makro SCENE jest w engine/SceneMacro.hpp - ten z kolei dołącza Engine.hpp, a do
 * gry trafia przez nagłówek zbiorczy `<raycade>`.
 *
 * Implementacja jest podzielona tematycznie na pliki w src/engine/:
 * - Engine.cpp - cykl życia, pętla Run(), Log() i RaylibLogLevel(), Clear(),
 * - EngineWindow.cpp - konfiguracja okna (bez skalowania), stan okna i monitory, sterowanie
 *   oknem,
 * - EngineScaling.cpp - Resolution(), ScaleMode(), SmoothScaling(), PostShader(), ViewSize(),
 * - EngineInput.cpp - ExitKey(), kursor i pady, MousePosition(), nagrywanie wejścia,
 * - EngineScenes.cpp - sceny,
 * - EngineSystem.cpp - schowek oraz czas, losowość i różne (w tym Screenshot(),
 *   EventWaiting()),
 * - EngineFiles.cpp - pliki, katalogi i zasoby (Assets()).
 */
#pragma once

#include <raylib.h>

#include <cstdint>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <raycade/input/Input.hpp>
#include <raycade/render/Assets.hpp>
#include <raycade/render/Bitmap.hpp>
#include <raycade/render/PrimitiveRenderer.hpp>
#include <raycade/render/RenderTarget.hpp>
#include <raycade/render/ShaderProgram.hpp>
#include <raycade/scene/Scene.hpp>

#include <raycade/engine/EngineTypes.hpp>

/**
 * @brief Główny obiekt silnika zarządzający oknem, scenami, wejściem i pętlą gry.
 * @details Silnik jest singletonem: Engine::Get() zwraca zawsze tę samą instancję.
 * Metody konfiguracji zwracają Engine&, dlatego można je łączyć w łańcuch.
 * Ustawienia okna można zmieniać także po Init() - zmieniają wtedy okno na bieżąco.
 *
 * W każdej klatce Run() wykonuje kolejno: globalne OnUpdate(), aktualizację aktywnej
 * sceny, czyszczenie ekranu, rysowanie sceny, globalne OnDraw() i zamianę buforów.
 *
 * Silnik jest korzeniem łańcucha kontekstu (Contextable): wartości z
 * Engine::Get().ProvideContext<T>() widzą wszystkie sceny i obiekty. Można je udostępnić przed
 * Init(); są usuwane w Shutdown(), przed zamknięciem okna.
 *
 * Obsługa błędów jest jednolita w obrębie rodzaju metody:
 * - konfiguracja i polecenia (settery, SwitchScene(), LoadScene(), Maximize() itd.) zwracają
 *   Engine& i niepoprawne wywołanie tylko zapisują w logu (LogLevel::Warning albo Error),
 *   żeby nie przerywać łańcucha wywołań,
 * - operacje na plikach i nagraniach zwracają wynik: `bool` (SaveText(), StopRecording(),
 *   Replay()) albo `std::optional` (LoadText()),
 * - Init() zapisuje błąd w logu, a wynik podaje IsReady(),
 * - wyjątek (std::out_of_range) rzuca tylko Scene(), bo musi zwrócić referencję do sceny,
 *   której nie ma.
 *
 * @code{.cpp}
 * Engine& engine = Engine::Get().Title("Gra").Window(800, 600).Fps(60).Init();
 * if (!engine.IsReady()) return 1;
 *
 * engine.CreateScene("menu", [&](Scene& scene) { // pierwsza scena staje się aktywna
 *     scene.Spawn<shapes::Circle>().Position({400, 300}).Radius(20).Paint(ORANGE).Filled();
 * });
 *
 * return engine
 *     .OnUpdate([&](float) {
 *         if (engine.KeyPressed(KEY_ENTER)) engine.SwitchScene("menu");
 *     })
 *     .Run();
 * @endcode
 */
class Engine : public Contextable {
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

    /** @brief Podaje tytuł okna. */
    const std::string& Title() const { return title_; }

    /**
     * @brief Ustawia rozmiar okna i wyłącza tryb pełnoekranowy.
     * @param width Szerokość w pikselach.
     * @param height Wysokość w pikselach.
     * @return Ten sam silnik.
     */
    Engine& Window(int width, int height);

    /**
     * @brief Ustawia rozdzielczość gry używaną przy ScaleMode() innym niż None.
     * @details Domyślnie równa rozmiarowi okna z Window() w chwili Init(). Gra zawsze rysuje
     * w tej rozdzielczości, a obraz jest skalowany na okno.
     * @param width Szerokość obszaru gry.
     * @param height Wysokość obszaru gry.
     * @return Ten sam silnik.
     */
    Engine& Resolution(int width, int height);

    /**
     * @brief Ustawia sposób dopasowania obrazu do okna (np. w trybie pełnoekranowym).
     * @param mode Tryb skalowania, patrz Scaling.
     * @return Ten sam silnik.
     */
    Engine& ScaleMode(Scaling mode);

    /** @brief Podaje tryb dopasowania obrazu do okna. */
    Scaling ScaleMode() const { return scaling_; }

    /**
     * @brief Wybiera filtr skalowania obrazu.
     * @details Domyślnie wyłączony: piksele pozostają ostre, co dobrze pokazuje własne
     * algorytmy rastrowe. Włączony wygładza obraz (filtr dwuliniowy).
     * @param value Czy wygładzać.
     * @return Ten sam silnik.
     */
    Engine& SmoothScaling(bool value = true);

    /** @brief Sprawdza, czy skalowany obraz jest wygładzany. */
    bool IsSmoothScaling() const { return smoothScaling_; }

    /**
     * @brief Ustawia kolor pasów wokół obrazu w trybach Fit i PixelPerfect.
     * @param color Kolor pasów (domyślnie czarny).
     * @return Ten sam silnik.
     */
    Engine& Letterbox(Color color) {
        letterbox_ = color;
        return *this;
    }

    /** @brief Podaje kolor pasów wokół obrazu. */
    Color Letterbox() const { return letterbox_; }

    /**
     * @brief Podaje rozmiar obszaru, w którym rysuje gra.
     * @return Rozdzielczość gry przy włączonym skalowaniu, w przeciwnym razie rozmiar okna.
     */
    Vector2 ViewSize() const;

    /**
     * @brief Sprawdza, czy włączony jest tryb pełnoekranowy.
     * @return Stan ustawienia Fullscreen().
     */
    bool IsFullscreen() const { return fullscreen_; }

    /**
     * @brief Włącza lub wyłącza tryb pełnoekranowy.
     * @details Po Init() okno najpierw dostaje natywną rozdzielczość monitora - inaczej raylib
     * przełączyłby monitor w tryb o rozmiarze okna (np. 1000x700). Z ScaleMode() obraz gry
     * jest wtedy skalowany na cały ekran.
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
     * @brief Podaje docelową liczbę klatek na sekundę.
     * @details Zmierzoną liczbę klatek podaje CurrentFps().
     */
    int Fps() const { return fps_; }

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

    /** @brief Sprawdza, czy włączona jest synchronizacja pionowa. */
    bool IsVSync() const { return (windowFlags_ & FLAG_VSYNC_HINT) != 0; }

    /**
     * @brief Pozwala użytkownikowi zmieniać rozmiar okna myszą.
     * @param value Czy okno ma zmienny rozmiar.
     * @return Ten sam silnik.
     */
    Engine& Resizable(bool value = true);

    /** @brief Sprawdza, czy okno ma zmienny rozmiar. */
    bool IsResizable() const { return (windowFlags_ & FLAG_WINDOW_RESIZABLE) != 0; }

    /**
     * @brief Włącza wygładzanie krawędzi (MSAA 4x).
     * @details Działa tylko przed Init() - później karta graficzna ma już utworzony bufor.
     * @param value Czy włączyć MSAA.
     * @return Ten sam silnik.
     */
    Engine& Msaa(bool value = true);

    /** @brief Sprawdza, czy włączone jest wygładzanie krawędzi (MSAA 4x). */
    bool IsMsaa() const { return (windowFlags_ & FLAG_MSAA_4X_HINT) != 0; }

    /**
     * @brief Włącza obsługę wysokiego DPI (ostry obraz przy skalowaniu systemu, np. 150%).
     * @details Działa tylko przed Init().
     * @param value Czy włączyć High DPI.
     * @return Ten sam silnik.
     */
    Engine& HighDpi(bool value = true);

    /** @brief Sprawdza, czy włączona jest obsługa wysokiego DPI. */
    bool IsHighDpi() const { return (windowFlags_ & FLAG_WINDOW_HIGHDPI) != 0; }

    /**
     * @brief Usuwa ramkę i pasek tytułu okna.
     * @param value Czy okno ma być bez ramki.
     * @return Ten sam silnik.
     */
    Engine& Undecorated(bool value = true);

    /** @brief Sprawdza, czy okno jest bez ramki. */
    bool IsUndecorated() const { return (windowFlags_ & FLAG_WINDOW_UNDECORATED) != 0; }

    /**
     * @brief Utrzymuje okno zawsze nad innymi oknami.
     * @param value Czy okno ma być zawsze na wierzchu.
     * @return Ten sam silnik.
     */
    Engine& AlwaysOnTop(bool value = true);

    /** @brief Sprawdza, czy okno jest zawsze na wierzchu. */
    bool IsAlwaysOnTop() const { return (windowFlags_ & FLAG_WINDOW_TOPMOST) != 0; }

    /**
     * @brief Ustawia minimalny rozmiar okna (przy zmiennym rozmiarze).
     * @param width Minimalna szerokość.
     * @param height Minimalna wysokość.
     * @return Ten sam silnik.
     */
    Engine& MinSize(int width, int height);

    /**
     * @brief Ustawia maksymalny rozmiar okna (przy zmiennym rozmiarze).
     * @param width Maksymalna szerokość.
     * @param height Maksymalna wysokość.
     * @return Ten sam silnik.
     */
    Engine& MaxSize(int width, int height);

    /**
     * @brief Ustawia ikonę okna z pliku obrazu (np. PNG).
     * @details Można wywołać przed Init(). Błąd wczytania jest zapisywany w logu.
     * @param path Ścieżka pliku, np. AssetPath("ikona.png").
     * @return Ten sam silnik.
     */
    Engine& Icon(std::string path);

    /**
     * @brief Ustawia shader nakładany na cały obraz gry (post-processing: CRT, winieta,
     * skala szarości).
     * @details Działa przy skalowaniu (ScaleMode() inny niż Scaling::None): gra rysuje się
     * wtedy do tekstury w swojej rozdzielczości, a shader działa przy przenoszeniu jej do okna.
     * Bez skalowania shader nie jest używany (silnik zapisuje ostrzeżenie w logu).
     * @param shader Shader albo nullptr - bez efektu.
     * @return Ten sam silnik.
     */
    Engine& PostShader(std::shared_ptr<ShaderProgram> shader);

    /** @brief Podaje shader obrazu gry albo nullptr. */
    const std::shared_ptr<ShaderProgram>& PostShader() const { return postShader_; }

    /**
     * @brief Ustawia kolor czyszczenia ekranu na początku każdej klatki.
     * @param color Kolor tła.
     * @return Ten sam silnik.
     */
    Engine& Background(Color color) {
        background_ = color;
        return *this;
    }

    /** @brief Podaje kolor tła. */
    Color Background() const { return background_; }

    /** @} */

    /** @name Konfiguracja wejścia i logów */
    /** @{ */

    /**
     * @brief Włącza lub wyłącza odczyt klawiatury.
     * @param value Czy włączyć klawiaturę.
     * @return Ten sam silnik.
     */
    Engine& KeyboardEnabled(bool value = true) {
        input_.KeyboardEnabled(value);
        return *this;
    }

    /** @brief Sprawdza, czy odczyt klawiatury jest włączony. */
    bool IsKeyboardEnabled() const { return input_.IsKeyboardEnabled(); }

    /**
     * @brief Włącza lub wyłącza odczyt myszy.
     * @details Wyłączenie myszy wyłącza też zdarzenia myszy obiektów sceny.
     * @param value Czy włączyć mysz.
     * @return Ten sam silnik.
     */
    Engine& MouseEnabled(bool value = true) {
        input_.MouseEnabled(value);
        return *this;
    }

    /** @brief Sprawdza, czy odczyt myszy jest włączony. */
    bool IsMouseEnabled() const { return input_.IsMouseEnabled(); }

    /**
     * @brief Włącza lub wyłącza odczyt padów.
     * @param value Czy włączyć pady.
     * @return Ten sam silnik.
     */
    Engine& GamepadEnabled(bool value = true) {
        input_.GamepadEnabled(value);
        return *this;
    }

    /** @brief Sprawdza, czy odczyt padów jest włączony. */
    bool IsGamepadEnabled() const { return input_.IsGamepadEnabled(); }

    /**
     * @brief Włącza lub wyłącza odczyt ekranu dotykowego.
     * @param value Czy włączyć dotyk.
     * @return Ten sam silnik.
     */
    Engine& TouchEnabled(bool value = true) {
        input_.TouchEnabled(value);
        return *this;
    }

    /** @brief Sprawdza, czy odczyt ekranu dotykowego jest włączony. */
    bool IsTouchEnabled() const { return input_.IsTouchEnabled(); }

    /**
     * @brief Wybiera gesty rozpoznawane przez raylib (domyślnie wszystkie).
     * @details Wyłączenie zbędnych gestów usuwa pomyłki, np. GESTURE_HOLD przy zwykłym
     * kliknięciu. Gesty odczytuje Input::GestureDetected() i bind::Gesture().
     * @param flags Suma flag GESTURE_*, np. `GESTURE_TAP | GESTURE_SWIPE_LEFT`.
     * @return Ten sam silnik.
     */
    Engine& Gestures(unsigned int flags) {
        gestures_ = flags;
        ::SetGesturesEnabled(flags);
        return *this;
    }

    /** @brief Podaje flagi rozpoznawanych gestów (GESTURE_*). */
    unsigned int Gestures() const { return gestures_; }

    /**
     * @brief Ustawia klawisz zamykający okno (domyślnie ESC).
     * @details Można wywołać przed Init(). KEY_NULL (0) wyłącza zamykanie klawiszem.
     * @param key Kod klawisza raylib.
     * @return Ten sam silnik.
     */
    Engine& ExitKey(int key);

    /** @brief Podaje klawisz zamykający okno. */
    int ExitKey() const { return exitKey_; }

    /**
     * @brief Ustawia ścieżkę pliku diagnostycznego.
     * @param path Ścieżka pliku; pusty napis wyłącza zapis do pliku.
     * @return Ten sam silnik.
     */
    Engine& LogFile(std::string path) {
        logFile_ = std::move(path);
        return *this;
    }

    /**
     * @brief Ustawia, od jakiego poziomu komunikaty samego raylib trafiają do logu.
     * @details Komunikaty raylib są przekazywane do Log() (z przedrostkiem "[raylib]"), więc
     * lądują także w pliku logu. Domyślnie LOG_INFO; LOG_WARNING wycisza informacje
     * o starcie, a LOG_NONE - wszystko.
     * @param level Poziom raylib (LOG_*).
     * @return Ten sam silnik.
     */
    Engine& RaylibLogLevel(int level);

    /** @} */

    /** @name Logika globalna (niezależna od aktywnej sceny) */
    /** @{ */

    /**
     * @brief Ustawia globalną logikę wykonywaną ze stałym krokiem czasu.
     * @details Patrz FixedStep() i Scene::OnFixedUpdate(). Wykonuje się przed krokiem
     * stałym aktywnej sceny.
     * @param update Funkcja otrzymująca stały krok w sekundach.
     * @return Ten sam silnik.
     */
    Engine& OnFixedUpdate(std::function<void(float step)> update) {
        fixedUpdate_ = std::move(update);
        return *this;
    }

    /**
     * @brief Ustawia długość kroku stałego (OnFixedUpdate() silnika i scen).
     * @details Logika ze zmiennym dt (OnUpdate()) daje inne wyniki przy 30 i przy 144 FPS,
     * np. skok o innej wysokości albo pocisk przelatujący przez cienką ścianę. Krok stały
     * liczy fizykę zawsze tym samym dt: silnik zbiera czas klatek i wykonuje tyle kroków, ile
     * się w nim zmieściło (najwyżej MaxFixedSteps() na klatkę - przy wolnym komputerze gra
     * zwalnia, zamiast się zawiesić). Resztę czasu podaje FixedAlpha().
     * @param seconds Krok w sekundach (domyślnie 1/60); 0 wyłącza krok stały.
     * @return Ten sam silnik.
     */
    Engine& FixedStep(float seconds) {
        fixedStep_ = seconds > 0.0f ? seconds : 0.0f;
        fixedAccumulator_ = 0.0f;
        return *this;
    }

    /** @brief Podaje długość kroku stałego w sekundach (0 - wyłączony). */
    float FixedStep() const { return fixedStep_; }

    /**
     * @brief Ustawia największą liczbę kroków stałych w jednej klatce.
     * @param steps Liczba kroków (domyślnie 5, co najmniej 1).
     * @return Ten sam silnik.
     */
    Engine& MaxFixedSteps(int steps) {
        maxFixedSteps_ = steps < 1 ? 1 : steps;
        return *this;
    }

    /** @brief Podaje największą liczbę kroków stałych w jednej klatce. */
    int MaxFixedSteps() const { return maxFixedSteps_; }

    /**
     * @brief Podaje, jaka część kroku stałego upłynęła od ostatniego wykonanego kroku.
     * @details Do płynnego rysowania przy kroku stałym dłuższym od klatki: pozycję rysuje się
     * jako `poprzednia + (bieżąca - poprzednia) * FixedAlpha()`.
     * @return Wartość z przedziału [0, 1].
     */
    float FixedAlpha() const { return fixedStep_ > 0.0f ? fixedAccumulator_ / fixedStep_ : 0.0f; }

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

    /**
     * @brief Ustawia reakcję na pliki przeciągnięte na okno.
     * @param callback Funkcja otrzymująca ścieżki upuszczonych plików.
     * @return Ten sam silnik.
     */
    Engine& OnFilesDropped(std::function<void(const std::vector<std::string>&)> callback) {
        filesDropped_ = std::move(callback);
        return *this;
    }

    /** @} */

    /** @name Cykl życia */
    /** @{ */

    /**
     * @brief Otwiera okno i inicjalizuje silnik, jeśli jeszcze nie jest gotowy.
     * @details Błąd utworzenia okna jest zapisywany w logu.
     * @return Ten sam silnik; wynik można sprawdzić przez IsReady().
     */
    Engine& Init();

    /**
     * @brief Sprawdza wynik inicjalizacji.
     * @return true, gdy okno jest gotowe.
     */
    bool IsReady() const { return initialized_; }

    /**
     * @brief Uruchamia pętlę gry aż do zamknięcia okna, a potem sprząta.
     * @return 0 po normalnym zakończeniu albo 1 po błędzie inicjalizacji (kod wyjścia dla main).
     */
    int Run();

    /**
     * @brief Kończy pętlę gry po bieżącej klatce (np. przycisk "Wyjście" w menu).
     * @details Klatka jest dokończona (rysowanie, zmiany scen), potem Run() sprząta i zwraca 0
     * - tak samo jak po zamknięciu okna.
     * @return Ten sam silnik.
     */
    Engine& Quit() {
        quit_ = true;
        return *this;
    }

    /** @brief Sprawdza, czy zlecono zakończenie pętli (Quit()). */
    bool IsQuitting() const { return quit_; }

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
    Engine& CreateScene(const std::string& name, const std::function<void(::Scene&)>& setup);

    /**
     * @brief Tworzy scenę albo zwraca już istniejącą o tej nazwie.
     * @details Pierwsza utworzona scena staje się aktywna.
     * @param name Nazwa sceny.
     * @return Referencja do sceny.
     */
    ::Scene& CreateScene(const std::string& name);

    /**
     * @brief Rejestruje konfigurację sceny wczytywanej leniwie.
     * @details Scena powstaje dopiero przy pierwszym wejściu na nią, zawsze po Init(), więc
     * w konfiguracji można wczytywać tekstury. Zwykle wywoływane przez makro SCENE.
     * Ponowna rejestracja tej samej nazwy jest ignorowana i zapisywana w logu.
     * @param name Nazwa sceny.
     * @param setup Funkcja konfigurująca scenę.
     * @return Ten sam silnik.
     */
    Engine& RegisterScene(std::string name, std::function<void(::Scene&)> setup);

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
    ::Scene& Scene(const std::string& name);

    /**
     * @brief Sprawdza, czy scena została już wczytana.
     * @param name Nazwa sceny.
     * @return true, gdy scena istnieje w pamięci.
     */
    bool IsSceneLoaded(const std::string& name) const { return scenes_.contains(name); }

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
     * @warning Po zwolnieniu referencje do sceny i jej obiektów (np. z Scene()) są
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
     * @brief Pobiera pozycję kursora.
     * @details We współrzędnych gry: przy ScaleMode() innym niż None są one przeliczone
     * z pikseli okna na rozdzielczość gry (ViewSize()).
     * @return Współrzędne kursora albo {0, 0}, gdy mysz jest wyłączona.
     */
    Vector2 MousePosition() const { return input_.MousePosition(); }

    /**
     * @brief Udostępnia pełny odczyt wejścia: klawiatura, mysz, pady i dotyk.
     * @details Powyższe metody to skróty najczęstszych zapytań; resztę (np. KeyReleased(),
     * MouseWheel(), GamepadAxis()) daje Input.
     * @return Stan wejścia (ten sam, który obiekty dostają w Tick()).
     */
    const ::Input& Input() const { return input_; }

    /** @} */

    /**
     * @name Akcje i osie wejścia
     * Nazwy zamiast konkretnych klawiszy: gra pyta o "skok", a nie o spację. Dzięki temu jedna
     * akcja może mieć kilka źródeł (klawiatura, mysz, pad), a sterowanie da się zmienić
     * w jednym miejscu. Odczyt: Input::ActionPressed(), Input::Axis() (także w Tick()).
     * @code{.cpp}
     * engine.BindAction("skok", {bind::Key(KEY_SPACE), bind::Pad(GAMEPAD_BUTTON_RIGHT_FACE_DOWN)})
     *     .BindAxis("ruch_x", {bind::Key(KEY_A), bind::Key(KEY_LEFT)},
     *               {bind::Key(KEY_D), bind::Key(KEY_RIGHT)},
     *               {bind::PadAxis(GAMEPAD_AXIS_LEFT_X, 1.0f)});
     *
     * // w Tick(dt, input):
     * if (input.ActionPressed("skok")) Jump();
     * Translate({input.Axis("ruch_x") * speed * dt, 0});
     * @endcode
     */
    /** @{ */

    /**
     * @brief Ustawia źródła akcji (zastępuje poprzednie).
     * @param action Nazwa akcji.
     * @param bindings Źródła z przestrzeni nazw bind.
     * @return Ten sam silnik.
     */
    Engine& BindAction(const std::string& action, std::vector<InputBinding> bindings) {
        input_.BindAction(action, std::move(bindings));
        return *this;
    }

    /**
     * @brief Ustawia oś: źródła na minus, na plus i osie analogowe pada.
     * @param axis Nazwa osi.
     * @param negative Źródła dające -1 (np. A, strzałka w lewo).
     * @param positive Źródła dające +1 (np. D, strzałka w prawo).
     * @param analog Osie pada: bind::PadAxis(GAMEPAD_AXIS_*, kierunek); kierunek -1 odwraca oś.
     * @param deadzone Wychylenia analogowe mniejsze od tej wartości są pomijane.
     * @return Ten sam silnik.
     */
    Engine& BindAxis(
        const std::string& axis,
        std::vector<InputBinding> negative,
        std::vector<InputBinding> positive,
        std::vector<InputBinding> analog = {},
        float deadzone = 0.15f
    ) {
        input_.BindAxis(
            axis, std::move(negative), std::move(positive), std::move(analog), deadzone
        );
        return *this;
    }

    /**
     * @brief Usuwa akcję albo oś o podanej nazwie.
     * @param name Nazwa akcji lub osi.
     * @return Ten sam silnik.
     */
    Engine& Unbind(const std::string& name) {
        input_.Unbind(name);
        return *this;
    }

    /** @brief Skrót: Input::ActionDown(). */
    bool ActionDown(const std::string& action) const { return input_.ActionDown(action); }
    /** @brief Skrót: Input::ActionPressed(). */
    bool ActionPressed(const std::string& action) const { return input_.ActionPressed(action); }
    /** @brief Skrót: Input::ActionReleased(). */
    bool ActionReleased(const std::string& action) const { return input_.ActionReleased(action); }
    /** @brief Skrót: Input::Axis(). */
    float Axis(const std::string& axis) const { return input_.Axis(axis); }

    /** @} */

    /**
     * @name Kursor i pady (sterowanie)
     * Działają po Init(); wcześniej są ignorowane (z wpisem w logu).
     */
    /** @{ */

    /**
     * @brief Przenosi kursor w podane miejsce.
     * @param position Nowa pozycja kursora we współrzędnych gry (jak MousePosition()).
     * @return Ten sam silnik.
     */
    Engine& MousePosition(Vector2 position);

    /**
     * @brief Ustawia wygląd kursora systemowego.
     * @param cursor Kod raylib (MOUSE_CURSOR_DEFAULT, MOUSE_CURSOR_POINTING_HAND itd.).
     * @return Ten sam silnik.
     */
    Engine& MouseCursor(int cursor);

    /**
     * @brief Ukrywa albo pokazuje kursor nad oknem.
     * @param value true - ukryj, false - pokaż.
     * @return Ten sam silnik.
     */
    Engine& CursorHidden(bool value = true);

    /**
     * @brief Blokuje kursor w oknie (ukryty, ruch tylko przez MouseDelta()) albo go zwalnia.
     * @details Przydatne do sterowania kamerą myszą.
     * @param value true - zablokuj, false - zwolnij.
     * @return Ten sam silnik.
     */
    Engine& CursorLocked(bool value = true);

    /** @brief Sprawdza, czy kursor jest ukryty (także gdy jest zablokowany). */
    bool IsCursorHidden() const;

    /** @brief Sprawdza, czy kursor jest zablokowany w oknie (CursorLocked()). */
    bool IsCursorLocked() const { return cursorLocked_; }

    /** @brief Sprawdza, czy kursor znajduje się nad oknem. */
    bool IsCursorOnScreen() const;

    /**
     * @brief Włącza wibrację pada.
     * @param gamepad Numer pada.
     * @param left Siła lewego silnika (0-1).
     * @param right Siła prawego silnika (0-1).
     * @param seconds Czas trwania w sekundach.
     * @return Ten sam silnik.
     */
    Engine& GamepadVibration(int gamepad, float left, float right, float seconds);

    /**
     * @brief Wczytuje mapowania padów w formacie SDL_GameControllerDB.
     * @param mappings Treść mapowań.
     * @return Liczba wczytanych mapowań albo 0.
     */
    int GamepadMappings(const std::string& mappings);

    /** @} */

    /**
     * @name Stan okna i monitory
     * Przed Init() zwracają wartości domyślne (0, false, {0, 0}).
     */
    /** @{ */

    /**
     * @brief Szerokość okna (w punktach ekranu, bez skali DPI).
     * @details Przy ScaleMode() innym niż None gra rysuje w rozdzielczości ViewSize(), a nie
     * w rozmiarze okna - do układania obiektów służy wtedy ViewSize().
     */
    int ScreenWidth() const;
    /** @brief Wysokość okna (w punktach ekranu, bez skali DPI); patrz ScreenWidth(). */
    int ScreenHeight() const;
    /** @brief Szerokość bufora rysowania w pikselach (z uwzględnieniem DPI). */
    int RenderWidth() const;
    /** @brief Wysokość bufora rysowania w pikselach (z uwzględnieniem DPI). */
    int RenderHeight() const;
    /** @brief Czy rozmiar okna zmienił się w poprzedniej klatce. */
    bool IsWindowResized() const;
    /** @brief Czy okno ma fokus. */
    bool IsWindowFocused() const;
    /** @brief Czy okno jest ukryte. */
    bool IsWindowHidden() const;
    /** @brief Czy okno jest zminimalizowane. */
    bool IsWindowMinimized() const;
    /** @brief Czy okno jest zmaksymalizowane. */
    bool IsWindowMaximized() const;
    /**
     * @brief Sprawdza dowolną flagę stanu okna raylib.
     * @details Ogólna wersja zapytań powyżej, np. dla FLAG_WINDOW_UNFOCUSED,
     * FLAG_WINDOW_TRANSPARENT albo FLAG_BORDERLESS_WINDOWED_MODE.
     * @param flag Flaga FLAG_*.
     * @return Czy flaga jest włączona (false przed Init()).
     */
    bool IsWindowState(unsigned int flag) const;
    /** @brief Pozycja okna na pulpicie. */
    Vector2 WindowPosition() const;
    /** @brief Skala DPI okna (np. {1.5, 1.5} przy 150%). */
    Vector2 WindowScaleDpi() const;

    /** @brief Liczba podłączonych monitorów. */
    int MonitorCount() const;
    /** @brief Numer monitora, na którym jest okno. */
    int CurrentMonitor() const;
    /** @brief Pozycja monitora na wirtualnym pulpicie. */
    Vector2 MonitorPosition(int monitor) const;
    /** @brief Szerokość trybu wideo monitora w pikselach. */
    int MonitorWidth(int monitor) const;
    /** @brief Wysokość trybu wideo monitora w pikselach. */
    int MonitorHeight(int monitor) const;
    /** @brief Fizyczna szerokość monitora w milimetrach. */
    int MonitorPhysicalWidth(int monitor) const;
    /** @brief Fizyczna wysokość monitora w milimetrach. */
    int MonitorPhysicalHeight(int monitor) const;
    /** @brief Częstotliwość odświeżania monitora w Hz. */
    int MonitorRefreshRate(int monitor) const;
    /** @brief Nazwa monitora (albo pusty napis). */
    std::string MonitorName(int monitor) const;

    /** @} */

    /**
     * @name Sterowanie oknem
     * WindowPosition() i Opacity() można wywołać przed Init() - zostaną zastosowane przy
     * otwarciu okna. Pozostałe działają po Init().
     */
    /** @{ */

    /**
     * @brief Przenosi okno na wybrany monitor (w pełnym ekranie - pełny ekran na tym monitorze).
     * @details Przed Init() wybór jest zapamiętywany i stosowany zaraz po utworzeniu okna.
     * Numery monitorów: od 0 do MonitorCount() - 1.
     * @param monitor Numer monitora.
     * @return Ten sam silnik.
     */
    Engine& Monitor(int monitor);

    /**
     * @brief Podaje numer monitora okna.
     * @details Po Init() - monitor, na którym okno faktycznie jest (CurrentMonitor()), przed
     * Init() - wybrany przez Monitor(int) albo 0.
     */
    int Monitor() const { return initialized_ ? CurrentMonitor() : monitor_.value_or(0); }

    /**
     * @brief Ustawia pozycję okna na pulpicie.
     * @param position Pozycja lewego górnego rogu okna w pikselach pulpitu (jak WindowPosition()).
     * @return Ten sam silnik.
     */
    Engine& WindowPosition(Vector2 position);

    /**
     * @brief Ustawia przezroczystość całego okna.
     * @param opacity Od 0 (niewidoczne) do 1 (nieprzezroczyste).
     * @return Ten sam silnik.
     */
    Engine& Opacity(float opacity);

    /** @brief Maksymalizuje okno (wymaga okna o zmiennym rozmiarze). */
    Engine& Maximize();
    /** @brief Minimalizuje okno. */
    Engine& Minimize();
    /** @brief Przywraca zminimalizowane albo zmaksymalizowane okno. */
    Engine& Restore();
    /** @brief Przełącza tryb okna bez ramki na cały ekran (borderless windowed). */
    Engine& ToggleBorderless();
    /** @brief Przenosi fokus na okno. */
    Engine& Focus();

    /** @} */

    /** @name Schowek */
    /** @{ */

    /**
     * @brief Pobiera tekst ze schowka systemowego.
     * @return Tekst albo pusty napis (także przed Init()).
     */
    std::string ClipboardText() const;

    /**
     * @brief Kopiuje tekst do schowka systemowego.
     * @param text Tekst (UTF-8).
     * @return Ten sam silnik.
     */
    Engine& ClipboardText(const std::string& text);

    /**
     * @brief Pobiera obraz ze schowka jako bitmapę.
     * @details W raylib 5.5 działa tylko na Windows; na innych systemach (i przed Init())
     * zwraca pustą bitmapę - sprawdź Width() > 0.
     * @return Bitmapa z obrazem albo pusta.
     */
    Bitmap ClipboardImage() const;

    /** @} */

    /** @name Czas, losowość i różne */
    /** @{ */

    /**
     * @brief Podaje czas od otwarcia okna.
     * @details Zegar raylib (GetTime()) - rośnie także podczas wczytywania scen. Przydaje się
     * do animacji zależnych od czasu (miganie, fale) i do mierzenia odstępów.
     * @return Czas w sekundach albo 0 przed Init().
     */
    double Time() const;

    /**
     * @brief Podaje czas trwania bieżącej klatki - to samo dt, które dostaje OnUpdate().
     * @details Bez czasu wczytywania scen i ograniczone przez MaxDeltaTime(). Pozwala
     * odczytać dt poza callbackami (np. w Draw()).
     * @return Czas klatki w sekundach.
     */
    float FrameTime() const { return frameTime_; }

    /**
     * @brief Podaje numer bieżącej klatki.
     * @return Liczba klatek od uruchomienia Run(), od 0 dla pierwszej klatki.
     */
    std::uint64_t FrameCount() const { return frameCount_; }

    /**
     * @brief Ogranicza czas klatki przekazywany do logiki gry.
     * @details Po zatrzymaniu programu w debuggerze, przeciąganiu okna albo chwilowym
     * przycięciu systemu dt może urosnąć do kilku sekund, a obiekty "przeskoczyłyby" przez
     * ściany. Dłuższa klatka jest liczona jako @p seconds (gra na moment zwalnia zamiast
     * skakać).
     * @param seconds Największe dt w sekundach (domyślnie 0.1); 0 wyłącza ograniczenie.
     * @return Ten sam silnik.
     */
    Engine& MaxDeltaTime(float seconds) {
        maxDeltaTime_ = seconds > 0.0f ? seconds : 0.0f;
        return *this;
    }

    /** @brief Podaje ograniczenie czasu klatki (0 - bez ograniczenia). */
    float MaxDeltaTime() const { return maxDeltaTime_; }

    /**
     * @brief Pobiera zmierzoną liczbę klatek na sekundę.
     * @details W odróżnieniu od Fps(), które podaje wartość docelową.
     * @return Średnia z ostatnich klatek.
     */
    int CurrentFps() const;

    /**
     * @brief Usypia program na podany czas.
     * @param seconds Czas w sekundach.
     * @return Ten sam silnik.
     */
    Engine& Wait(double seconds);

    /**
     * @brief Ustawia ziarno generatora liczb losowych (powtarzalne wyniki).
     * @param seed Ziarno.
     * @return Ten sam silnik.
     */
    Engine& RandomSeed(unsigned int seed);

    /**
     * @brief Losuje liczbę całkowitą z przedziału domkniętego.
     * @param min Dolna granica.
     * @param max Górna granica.
     * @return Wylosowana liczba.
     */
    int RandomValue(int min, int max) const;

    /**
     * @brief Losuje liczbę zmiennoprzecinkową z przedziału [min, max].
     * @details Korzysta z tego samego generatora co RandomValue(), więc RandomSeed() działa
     * dla obu. Rozdzielczość: milionowa część przedziału.
     * @param min Dolna granica.
     * @param max Górna granica.
     * @return Wylosowana liczba.
     */
    float RandomFloat(float min, float max) const;

    /**
     * @brief Losuje ciąg różnych liczb całkowitych z przedziału [min, max] (bez powtórzeń).
     * @details Przydaje się do tasowania: kolejność kart, pozycje przeszkód bez nakładania.
     * Korzysta z generatora RandomValue(), więc RandomSeed() działa i tutaj.
     * @param count Liczba elementów (najwyżej max - min + 1).
     * @param min Dolna granica.
     * @param max Górna granica.
     * @return Wylosowane liczby albo pusta lista, gdy @p count przekracza rozmiar przedziału.
     */
    std::vector<int> RandomSequence(unsigned int count, int min, int max) const;

    /**
     * @brief Zapisuje zrzut ekranu do pliku na końcu bieżącej klatki.
     * @details Zrzut obejmuje całą narysowaną klatkę (scenę, HUD i pasy przy skalowaniu),
     * niezależnie od tego, czy Screenshot() wywołano w OnUpdate(), OnDraw() czy w zdarzeniu
     * obiektu. Wynik zapisu trafia do logu.
     * @param path Ścieżka pliku (także z katalogami); format wynika z rozszerzenia, np. ".png".
     * @return Ten sam silnik.
     */
    Engine& Screenshot(const std::string& path);

    /**
     * @brief Otwiera adres w domyślnej przeglądarce.
     * @param url Adres (http:// albo https://).
     * @return Ten sam silnik.
     */
    Engine& OpenUrl(const std::string& url);

    /**
     * @brief Włącza czekanie na zdarzenia zamiast ciągłego odświeżania.
     * @details Program zużywa mniej procesora, ale klatki pojawiają się tylko przy wejściu
     * użytkownika - dobre dla narzędzi, złe dla animacji.
     * @param value Czy czekać na zdarzenia.
     * @return Ten sam silnik.
     */
    Engine& EventWaiting(bool value = true);

    /** @brief Sprawdza, czy silnik czeka na zdarzenia zamiast ciągle odświeżać. */
    bool IsEventWaiting() const { return eventWaiting_; }

    /** @} */

    /** @name Pliki i katalogi */
    /** @{ */

    /** @brief Katalog, w którym leży plik wykonywalny (z ukośnikiem na końcu). */
    std::string ApplicationDirectory() const;
    /** @brief Bieżący katalog roboczy. */
    std::string WorkingDirectory() const;
    /** @brief Sprawdza, czy plik istnieje. */
    bool FileExists(const std::string& path) const;
    /** @brief Sprawdza, czy katalog istnieje. */
    bool DirectoryExists(const std::string& path) const;
    /** @brief Czas ostatniej modyfikacji pliku (sekundy od epoki Uniksa) albo 0. */
    long FileModTime(const std::string& path) const;

    /**
     * @brief Zwraca pełną ścieżkę zasobu z katalogu assets/ obok pliku wykonywalnego.
     * @details CMake kopiuje tam katalog assets/ gry, więc ścieżka działa niezależnie od tego,
     * z jakiego katalogu uruchomiono program.
     * @param name Nazwa pliku w assets/, np. "player.png" albo "dzwieki/skok.wav".
     * @return Ścieżka, np. "/home/.../build/assets/player.png".
     */
    std::string AssetPath(const std::string& name) const;

    /**
     * @brief Udostępnia pamięć podręczną zasobów (bitmapy, czcionki).
     * @details Nazwy względne są liczone od katalogu assets/ (AssetPath()). Ten sam plik
     * wczytuje się raz - kolejne prośby zwracają ten sam obiekt. Nieużywane zasoby są
     * zwalniane po zwolnieniu albo ponownym wczytaniu sceny, a wszystkie - w Shutdown(),
     * przed zamknięciem okna. Assets są też w kontekście silnika (`Context<Assets>()`).
     * @code{.cpp}
     * auto hero = engine.Assets().Bitmap("hero.png");
     * auto font = engine.Assets().Font("font.ttf");
     * @endcode
     * @return Pamięć podręczna zasobów.
     */
    ::Assets& Assets() { return assets_; }

    /** @copydoc Assets() */
    const ::Assets& Assets() const { return assets_; }

    /**
     * @brief Wczytuje cały plik tekstowy.
     * @param path Ścieżka pliku.
     * @return Treść albo brak wartości, gdy pliku nie da się odczytać.
     */
    std::optional<std::string> LoadText(const std::string& path) const;

    /**
     * @brief Zapisuje tekst do pliku (nadpisując go).
     * @param path Ścieżka pliku.
     * @param text Treść.
     * @return true, gdy zapis się udał.
     */
    bool SaveText(const std::string& path, const std::string& text) const;

    /**
     * @brief Wczytuje cały plik binarny (zapis gry, własny format poziomu).
     * @param path Ścieżka pliku.
     * @return Bajty pliku albo brak wartości, gdy pliku nie da się odczytać.
     */
    std::optional<std::vector<unsigned char>> LoadData(const std::string& path) const;

    /**
     * @brief Zapisuje bajty do pliku (nadpisując go).
     * @param path Ścieżka pliku.
     * @param data Dane do zapisania.
     * @return true, gdy zapis się udał.
     */
    bool SaveData(const std::string& path, std::span<const unsigned char> data) const;

    /**
     * @brief Listuje pliki w katalogu.
     * @details Do rozbioru ścieżek (nazwa, rozszerzenie, katalog nadrzędny) służy
     * std::filesystem::path - silnik nie powiela tych funkcji raylib (patrz
     * @ref konwencje "konwencje").
     * @code{.cpp}
     * for (const std::string& level : engine.ListDirectory(engine.AssetPath("levels"), ".lvl"))
     *     engine.Log("poziom: " + level);
     * @endcode
     * @param path Katalog.
     * @param filter Rozszerzenia oddzielone średnikami (np. ".png;.jpg"); "DIR" dołącza
     * katalogi; pusty napis - wszystkie pliki.
     * @param recursive Czy przeszukiwać podkatalogi.
     * @return Pełne ścieżki (pusta lista, gdy katalogu nie ma).
     */
    std::vector<std::string> ListDirectory(
        const std::string& path,
        const std::string& filter = "",
        bool recursive = false
    ) const;

    /**
     * @brief Tworzy katalog razem z brakującymi katalogami nadrzędnymi.
     * @param path Ścieżka katalogu.
     * @return true, gdy katalog istnieje po wywołaniu.
     */
    bool MakeDirectory(const std::string& path) const;

    /**
     * @brief Zmienia katalog roboczy programu (względem niego liczą się ścieżki względne).
     * @param path Nowy katalog roboczy.
     * @return true, gdy zmiana się udała.
     */
    bool ChangeDirectory(const std::string& path) const;

    /** @} */

    /**
     * @name Nagrywanie i odtwarzanie wejścia
     * Nagrywa klawiaturę, mysz i pady klatka po klatce, a potem odtwarza je, np. żeby pokazać
     * demo na obronie zawsze tak samo. Podczas odtwarzania prawdziwe wejście nadal działa.
     * @code{.cpp}
     * if (engine.KeyPressed(KEY_F5)) engine.StartRecording();
     * if (engine.KeyPressed(KEY_F6)) engine.StopRecording("demo.rae");
     * if (engine.KeyPressed(KEY_F7)) engine.Replay("demo.rae");
     * @endcode
     */
    /** @{ */

    /**
     * @brief Zaczyna nagrywanie wejścia.
     * @return Ten sam silnik.
     */
    Engine& StartRecording();

    /**
     * @brief Kończy nagrywanie i zapisuje je do pliku.
     * @param path Ścieżka pliku nagrania (format tekstowy raylib).
     * @return true, gdy zapis się udał.
     */
    bool StopRecording(const std::string& path);

    /**
     * @brief Odtwarza nagranie od następnej klatki.
     * @param path Ścieżka pliku nagrania.
     * @return true, gdy nagranie wczytano i ma zdarzenia.
     */
    bool Replay(const std::string& path);

    /** @brief Czy trwa nagrywanie. */
    bool IsRecording() const { return recording_; }
    /** @brief Czy trwa odtwarzanie. */
    bool IsReplaying() const { return replaying_; }

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
     * @brief Czyści teksturę renderowaną (np. zawartość visual::RenderGroup).
     * @param target Tekstura docelowa.
     * @param color Kolor czyszczenia.
     * @return Ten sam silnik.
     */
    Engine& Clear(const RenderTarget& target, Color color);

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
     * @details Każdy wpis zaczyna się znacznikiem poziomu, np. `[UWAGA] ...`. Komunikaty raylib
     * trafiają tu z tym samym znacznikiem i dopiskiem `[raylib]`.
     * @param level Poziom komunikatu.
     * @param message Treść komunikatu.
     * @return Ten sam silnik.
     */
    Engine& Log(LogLevel level, const std::string& message);

    /**
     * @brief Zapisuje komunikat informacyjny (LogLevel::Info).
     * @param message Treść komunikatu.
     * @return Ten sam silnik.
     */
    Engine& Log(const std::string& message) { return Log(LogLevel::Info, message); }

    /** @} */

  private:
    /** @brief Prywatny konstruktor - instancję zwraca wyłącznie Get(). */
    Engine();
    /** @brief Destruktor wywołuje Shutdown(). */
    ~Engine() { Shutdown(); }

    /**
     * @brief Sprawdza, czy okno istnieje; jeśli nie - zapisuje ostrzeżenie w logu.
     * @param function Nazwa wywołanej metody (do komunikatu).
     * @return true, gdy okno jest gotowe.
     */
    bool RequireWindow(const char* function);

    /**
     * @brief Ustawia lub zdejmuje flagę okna (przed Init() - zapamiętuje, po - zmienia okno).
     * @param flag Flaga raylib (FLAG_*).
     * @param value Czy flaga ma być ustawiona.
     * @param name Nazwa metody (do komunikatów).
     * @param onlyBeforeInit Czy flagi nie da się zmienić po Init() (MSAA, High DPI).
     * @return Ten sam silnik.
     */
    Engine& WindowFlag(unsigned int flag, bool value, const char* name, bool onlyBeforeInit);

    /** @brief Ustawia ikonę z icon_ (wymaga okna). */
    void ApplyIcon();

    /** @brief Obsługuje upuszczone pliki i odtwarzanie nagrania na początku klatki. */
    void PollSystemEvents();
    void SaveScreenshot();
    /** @brief Tworzy (albo odtwarza) teksturę gry zgodnie z bieżącymi ustawieniami. */
    void CreateTarget();

    /** @brief Zwalnia teksturę gry i przywraca przeliczanie myszy 1:1. */
    void ReleaseTarget();

    /** @brief Liczy prostokąt obrazu w oknie i przeliczenie myszy dla bieżącego rozmiaru okna. */
    void UpdateScaling();

    std::string title_ = "Gra";             /**< Tytuł okna. */
    int width_ = 800;                       /**< Szerokość okna w trybie okienkowym. */
    int height_ = 600;                      /**< Wysokość okna w trybie okienkowym. */
    bool fullscreen_ = false;               /**< Czy używać trybu pełnoekranowego. */
    int fps_ = 60;                          /**< Docelowa liczba klatek na sekundę. */
    Color background_ = BLACK;              /**< Kolor czyszczenia ekranu w każdej klatce. */
    ::Input input_;                         /**< Stan i ustawienia urządzeń wejścia. */
    int exitKey_ = KEY_ESCAPE;              /**< Klawisz zamykający okno. */
    std::optional<Vector2> windowPosition_; /**< Pozycja okna do ustawienia przy Init(). */
    std::optional<int> monitor_;            /**< Monitor do ustawienia przy Init(). */
    unsigned int gestures_ = 0b1111111111;  /**< Rozpoznawane gesty (domyślnie wszystkie). */
    std::optional<float> opacity_;          /**< Przezroczystość okna do ustawienia przy Init(). */
    bool cursorLocked_ = false;             /**< Czy kursor jest zablokowany (CursorLocked()). */
    bool eventWaiting_ = false;             /**< Czy włączone jest EventWaiting(). */
    unsigned int windowFlags_ = 0;          /**< Flagi okna (FLAG_*) z Resizable() itd. */
    std::optional<std::pair<int, int>> minSize_; /**< Minimalny rozmiar okna. */
    std::optional<std::pair<int, int>> maxSize_; /**< Maksymalny rozmiar okna. */
    std::string icon_;                           /**< Ścieżka ikony do ustawienia przy Init(). */
    int raylibLogLevel_ = LOG_INFO;              /**< Próg komunikatów raylib. */
    /** Reakcja na pliki upuszczone na okno. */
    std::function<void(const std::vector<std::string>&)> filesDropped_;
    AutomationEventList recordList_{}; /**< Bufor nagrywanego wejścia. */
    AutomationEventList replayList_{}; /**< Odtwarzane nagranie. */
    bool recording_ = false;           /**< Czy trwa nagrywanie. */
    bool replaying_ = false;           /**< Czy trwa odtwarzanie. */
    unsigned int replayIndex_ = 0;     /**< Następne zdarzenie do odtworzenia. */
    unsigned int replayFrame_ = 0;     /**< Klatka odtwarzania (od 0). */

    Scaling scaling_ = Scaling::None; /**< Tryb dopasowania obrazu do okna. */
    int viewWidth_ = 0;               /**< Szerokość gry; 0 = rozmiar okna przy Init(). */
    int viewHeight_ = 0;              /**< Wysokość gry; 0 = rozmiar okna przy Init(). */
    bool smoothScaling_ = false;      /**< Czy wygładzać skalowany obraz. */
    Color letterbox_ = BLACK;         /**< Kolor pasów wokół obrazu. */
    RenderTarget target_;             /**< Tekstura, do której rysuje gra przy skalowaniu. */
    Rectangle destination_{};         /**< Gdzie w oknie rysujemy teksturę gry. */
    std::shared_ptr<ShaderProgram> postShader_; /**< Shader całego obrazu gry. */
    std::string logFile_ = "engine.log";        /**< Ścieżka pliku logu; pusta wyłącza zapis. */

    std::function<void(float)> update_;            /**< Globalna logika co klatkę. */
    std::function<void(float)> fixedUpdate_;       /**< Globalna logika ze stałym krokiem. */
    float fixedStep_ = 1.0f / 60.0f;               /**< Długość kroku stałego; 0 - wyłączony. */
    float fixedAccumulator_ = 0.0f;                /**< Czas zebrany do następnego kroku. */
    int maxFixedSteps_ = 5;                        /**< Limit kroków stałych na klatkę. */
    std::function<void(PrimitiveRenderer&)> draw_; /**< Globalne rysowanie po scenie. */

    std::ofstream log_;        /**< Otwarty plik logu. */
    bool initialized_ = false; /**< Czy okno zostało utworzone. */
    bool quit_ = false;        /**< Czy zakończyć pętlę po bieżącej klatce. */
    std::string screenshot_;   /**< Ścieżka zrzutu do wykonania na końcu klatki. */

    ::Assets assets_;                       /**< Pamięć podręczna zasobów z plików. */
    std::map<std::string, ::Scene> scenes_; /**< Utworzone sceny według nazw. */
    std::string active_;                    /**< Nazwa aktywnej sceny. */
    std::string pending_; /**< Scena, na którą przełączamy po zakończeniu klatki. */
    std::string start_;   /**< Scena startowa wybrana przez StartScene(). */
    /** Konfiguracje scen zarejestrowanych makrem SCENE, wykonywane przy pierwszym wejściu. */
    std::map<std::string, std::function<void(::Scene&)>> registered_;
    PrimitiveRenderer primitives_; /**< Renderer używany do rysowania scen. */
    /** Czas wczytywania scen w bieżącej klatce, odejmowany od dt następnej. */
    double loadSeconds_ = 0.0;
    float frameTime_ = 0.0f;       /**< dt bieżącej klatki (po poprawkach). */
    float maxDeltaTime_ = 0.1f;    /**< Największe dt przekazywane do logiki; 0 - bez limitu. */
    std::uint64_t frameCount_ = 0; /**< Numer bieżącej klatki. */
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
    ::Scene* Load(const std::string& name);

    /**
     * @brief Wybiera i wczytuje scenę startową na początku Run().
     */
    void LoadStartScene();
};
