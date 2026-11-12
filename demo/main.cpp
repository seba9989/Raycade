/**
 * @file main.cpp
 * @brief Punkt wejścia: konfiguracja silnika, przełączanie scen i HUD.
 */

/**
 * @mainpage Raycade
 *
 * @section cel Cel projektu
 * Raycade (ray + cascade + arcade) to silnik 2D napisany w C++23 z użyciem raylib,
 * z API opartym na kaskadzie wywołań: `scene.Spawn<shapes::Circle>().Radius(20).Fill()`. Biblioteka
 * udostępnia sceny, obiekty z hierarchią dzieci, wejście z klawiatury i myszy oraz renderer
 * prymitywów. Program zawiera demonstrację funkcji z laboratoriów 1-4.
 *
 * @section uruchomienie Kompilacja i uruchomienie
 * W katalogu głównym projektu należy wykonać:
 * @code{.sh}
 * cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
 * cmake --build build
 * @endcode
 * Program wynikowy to `build/raycade-demo.exe` na Windows albo `build/raycade-demo` na Linux.
 * Jeśli raylib nie jest dostępny lokalnie, CMake pobiera wersję 5.5.
 *
 * @section struktura Struktura projektu
 * - `engine/` - biblioteka statyczna Raycade (cel CMake `raycade::raycade`):
 *   nagłówek zbiorczy `engine/umbrella/raycade`, pozostałe publiczne nagłówki
 *   w `engine/include/raycade/` w katalogach-warstwach (`math`, `render`, `input`, `entity`,
 *   `shapes`, `visual`, `ui`, `scene`), implementacja w `engine/src/` w tym samym układzie.
 * - `demo/` - program demonstracyjny: ten plik i sceny z laboratoriów (`demo/scenes/`).
 * - `docs/` - konfiguracja Doxygena i @ref konwencje "konwencje kodu".
 *
 * Gra dołącza jeden nagłówek `#include <raycade>`. Inny projekt CMake może użyć
 * biblioteki bez demo:
 * @code{.cmake}
 * add_subdirectory(raycade)   # demo budowane jest tylko w głównym projekcie
 * target_link_libraries(gra PRIVATE raycade::raycade)
 * @endcode
 * Sceny (makro SCENE) muszą leżeć w plikach programu, nie w bibliotece statycznej.
 *
 * @section budowa Najważniejsze części
 * - @ref Engine zarządza oknem, pętlą i przełączaniem scen.
 * - @ref Scene przechowuje obiekty aktywnego ekranu.
 * - @ref Entity łączy drzewo obiektów z @ref Updatable, @ref Drawable i @ref Transformable;
 *   ich settery i zdarzenia zwracają konkretny typ figury dzięki deducing this (C++23).
 * - @ref PrimitiveRenderer rysuje na ekranie albo na @ref Bitmap.
 * - @ref visual::Canvas udostępnia bitmapę do algorytmów wypełniania.
 *
 * @section algorytmy Algorytmy laboratoriów
 * Własny algorytm przyrostowy odcinka, okrąg z ośmiokrotną symetrią, elipsa
 * z czterokrotną symetrią, kontrola przecięć wielokąta, wypełnianie scanline
 * oraz iteracyjne flood fill i boundary fill są opisane przy metodach
 * @ref PrimitiveRenderer. W scenie `figury` porównano krzywe z raylib.
 *
 * @section obsluga Sceny demonstracyjne
 * Klawisze 1-9 wybierają `menu`, `prymitywy`, `figury`, `transformacje`, `kontekst`,
 * `trafienia`, `kamera`, `efekty` i `formularz`, 0 - `sprite`, a minus - `fizyka` (lista jest
 * też na scenie `menu`). F11 przełącza pełny ekran. Okno ma zmienny rozmiar: gra rysuje zawsze w
 * 1000x700, a obraz jest skalowany do okna z zachowaniem proporcji
 * (Engine::ScaleMode(Scaling::Fit)), więc układ scen i kliknięcia działają przy każdym rozmiarze
 * okna i na pełnym ekranie. Na scenie `prymitywy` strzałki przesuwają prostokąt, L zmienia warstwę,
 * PPM przesuwa koniec odcinka, a spacja przełącza jego algorytm.
 * Na scenie `figury` LPM uruchamia flood fill na płótnie, Shift+LPM
 * uruchamia boundary fill, a R przywraca kontury.
 * Na scenie `transformacje` szary kontur pokazuje stan przed obrotem lub skalowaniem,
 * a kolorowa figura stan po; grupa obraca się razem z dziećmi. LPM zmienia kolor
 * obróconego prostokąta.
 * Na scenie `kontekst` LPM na kafelku zmienia motyw (Theme) całego panelu i podbija wynik
 * sceny, strzałki poruszają graczem, za którym gonią kółka znajdujące go przez kontekst,
 * a R wczytuje scenę od nowa - wynik sceny wraca do zera, a rekord z kontekstu silnika
 * zostaje.
 * Na scenie `trafienia` przeciąga się tylko najwyższą kartę, żółta ramka bez zdarzeń
 * przepuszcza kliknięcia do przycisku, okno z pustym OnClick blokuje przyciski pod sobą,
 * a robot złożony z kształtów reaguje jako całość (zdarzenia idą do rodzica).
 * Na scenie `kamera` WASD/strzałki poruszają graczem, za którym podąża kamera, kółko myszy
 * przybliża, Q/E obracają widok, F przełącza podążanie, a R resetuje kamerę. Skrzynie można
 * przeciągać, a monety klikać przy dowolnym przybliżeniu i obrocie; przyciski w rogu są
 * obiektami ekranu (HUD).
 * Na scenie `efekty` koło z arkusza klatek (visual::Sprite) obraca się animacją - LPM ją
 * wstrzymuje, X odbija drugie koło; lista w panelu z ClipRect() przewija się kółkiem myszy;
 * powierzchnia faluje shaderem, a G włącza szarość całego obrazu (Engine::PostShader());
 * piłki odbijają się z fizyką w kroku stałym (OnFixedUpdate()), LPM usuwa piłkę (Destroy()),
 * a B albo podwójne kliknięcie (gest) dodaje nową. Napisy sceny mają polskie znaki dzięki
 * czcionce `demo/assets/font.ttf` (DejaVu Sans, licencja w `demo/assets/FONT-LICENSE.txt`);
 * można ją podmienić na dowolny inny plik TTF (Typeface).
 * Na scenie `formularz` dwa pola tekstowe (ui::TextField) zbierają imię i e-mail: Tab
 * przechodzi między polami, Enter w imieniu przenosi do e-maila, a w e-mailu - tak jak
 * przycisk "Zapisz" - sprawdza dane i dopisuje osobę do listy. Pole z fokusem przejmuje
 * klawiaturę (cyfry nie przełączają wtedy scen), Escape zdejmuje fokus.
 * Scena `sprite` służy do testowania visual::Sprite i SpriteSheet. Arkusz wczytuje przez
 * Engine::Assets() z pliku `demo/assets/sprite-test.json` w formacie eksportu Aseprite (klatki
 * z czasami i tagi animacji), a obraz - z `sprite-test.png` wskazanego w JSON; podgląd arkusza
 * pokazuje ramką bieżącą klatkę. Postać sterują klawisze: Q, W, E, R, C animacje z tagów
 * (forward, pingpong, repeat, reverse), T lista klatek dopisana w kodzie, spacja pauza, S stop,
 * strzałki klatka i szybkość, X/Y odbicie, Z obrót, a L wczytuje obraz od nowa - po edycji PNG
 * w programie graficznym wszystkie sprite'y ze wspólnym arkuszem pokazują nową wersję. Są też
 * dziennik zdarzeń klatek, 16 sprite'ów z jednym arkuszem oraz przypadki brzegowe: prywatna
 * kopia opisu, Sheet() z nazwą obrazu i z nazwą JSON (te same obiekty z Assets), brakujący
 * plik i obrót wokół Origin().
 * Scena `fizyka` pokazuje kolizje: postać (CharacterBody) chodzi A/D albo strzałkami i skacze
 * spacją, a MoveAndSlide() zatrzymuje ją na colliderach różnych kształtów (Collider\<S\>:
 * prostokąty, trójkątna pochylnia, koło, obrócona platforma). Ruchoma platforma to StaticBody
 * przesuwany w kodzie, który wiezie stojącą na nim postać, szary blok zmienia kolor przy
 * zderzeniu, a monety i lawa to obszary (Area). Na brązową półkę (OneWay()) wskakuje się od
 * dołu, a S zeskakuje przez nią. Skrzynie i piłki to bryły sztywne (RigidBody) - spadają,
 * obracają się i układają w stos; gracz pcha skrzynie, a B tworzy piłkę pod kursorem.
 * Fioletowe oko rzuca promień do gracza (Scene::Raycast()) -
 * linia jest zielona, gdy nic go nie zasłania. F1 pokazuje obrysy colliderów, R wczytuje scenę
 * od nowa.
 *
 * @section dokumentacja Dokumentacja
 * Dokumentację HTML generuje polecenie `docs` (devenv) albo, w katalogu głównym projektu:
 * @code{.sh}
 * doxygen docs/Doxyfile
 * @endcode
 * Wynik trafia do `docs/html/index.html`.
 */

#include <string>

#include <raycade>

/**
 * @brief Konfiguruje silnik, ustawia logikę globalną i uruchamia pętlę gry.
 * @details Sceny rejestrują się same - patrz makro SCENE w plikach katalogu src/scenes.
 * @return Kod wyjścia zwrócony przez Engine::Run().
 */
int main() {
    Engine& engine = Engine::Get()
                         .Title("Raycade demo")
                         .VSync(false)      // V-Sync wyłączony; podwójne buforowanie działa zawsze
                         .Window(1000, 700) // albo .Fullscreen()
                         .MinSize(500, 350)
                         .Resolution(1000, 700)
                         .ScaleMode(Scaling::Fit)
                         .Background(RAYWHITE)
                         .StartScene("menu")
                         .Init();

    if (!engine.IsReady()) return 1;

    return engine
        .OnUpdate([&](float) {
            if (engine.KeyPressed(KEY_ONE)) engine.SwitchScene("menu");
            if (engine.KeyPressed(KEY_TWO)) engine.SwitchScene("prymitywy");
            if (engine.KeyPressed(KEY_THREE)) engine.SwitchScene("figury");
            if (engine.KeyPressed(KEY_FOUR)) engine.SwitchScene("transformacje");
            if (engine.KeyPressed(KEY_FIVE)) engine.SwitchScene("kontekst");
            if (engine.KeyPressed(KEY_SIX)) engine.SwitchScene("trafienia");
            if (engine.KeyPressed(KEY_SEVEN)) engine.SwitchScene("kamera");
            if (engine.KeyPressed(KEY_EIGHT)) engine.SwitchScene("efekty");
            if (engine.KeyPressed(KEY_NINE)) engine.SwitchScene("formularz");
            if (engine.KeyPressed(KEY_ZERO)) engine.SwitchScene("sprite");
            if (engine.KeyPressed(KEY_MINUS)) engine.SwitchScene("fizyka");
            // szarość całego obrazu (G) jest częścią sceny efektów - poza nią ją wyłączamy
            if (engine.ActiveScene() != "efekty" && engine.PostShader()) engine.PostShader(nullptr);
            if (engine.KeyPressed(KEY_F11)) engine.Fullscreen(!engine.IsFullscreen());
        })
        .OnDraw([&](PrimitiveRenderer&) {
            const std::string hud = "Scena: " + engine.ActiveScene() +
                                    "   [0-9, -] zmiana sceny (lista w menu)   [F11] pelny ekran";
            const int bottom = static_cast<int>(engine.ViewSize().y);
            ::DrawText(hud.c_str(), 20, bottom - 32, 18, DARKGRAY);
        })
        .Run();
}
