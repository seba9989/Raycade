# Konwencje kodu Raycade {#konwencje}

Zasady, których trzyma się API silnika. Nowy kod (także sceny gry) powinien je stosować,
żeby całość czytała się jednakowo.

## Układ plików

Nagłówki leżą w `engine/include/raycade/` w katalogach-warstwach. Każda warstwa dołącza tylko
warstwy wcześniejsze na liście:

| Katalog    | Zawartość                                                      | Zależy od               |
|------------|----------------------------------------------------------------|-------------------------|
| `math/`    | AffineMatrix2D, Point2D, LineSegment, algorytmy geometrii      | -                       |
| `render/`  | Bitmap, RenderTarget, ClipStack, ShaderProgram, PrimitiveRenderer | math                 |
| `input/`   | Input, akcje i osie                                            | -                       |
| `entity/`  | Entity i klasy bazowe (Transformable, Drawable, Updatable...)  | math, render, input     |
| `shapes/`  | kształty z przestrzeni nazw `shapes`                           | entity                  |
| `ui/`      | elementy interfejsu z przestrzeni nazw `ui`                    | entity                  |
| `scene/`   | Scene, SceneCamera                                             | entity                  |
| `Engine.hpp` | pętla, okno, sceny, zasoby                                   | scene, input, render    |

Nagłówek zbiorczy katalogu leży obok niego (`shapes.hpp` obok `shapes/`, `ui.hpp` obok
`ui/`). `<raycade>` (w `engine/umbrella/`) dołącza `Engine.hpp`, `shapes.hpp` i `ui.hpp`.
Źródła w `engine/src/` odwzorowują ten sam układ; nagłówki prywatne implementacji (np.
`src/render/primitive_renderer/Pixel.hpp`) zostają w `src/`.

W nagłówku zostaje tylko to, co musi lub powinno być inline: szablony (settery z deducing
this, `Spawn<T>()`, `Follow<T>()`), jedno- i dwuwierszowe gettery/settery oraz drobna
matematyka (`AffineMatrix2D`). Pozostałe ciała metod - w tym wirtualne `Draw()`, `Contains()`
i `Tick()` - są w odpowiadającym pliku `.cpp`. Typ potrzebny tylko w implementacji
deklaruje się zapowiadająco (`class SceneCamera;`), żeby nie łamać kolejności warstw.

Wewnątrz silnika nagłówki dołącza się zawsze pełną ścieżką: `#include <raycade/entity/Entity.hpp>`,
nigdy względną (`"../Entity.hpp"`). Ścieżka względna zostaje tylko dla nagłówków prywatnych
w `src/`. Katalogi mają nazwy małymi literami, pliki z klasami - PascalCase.

## Nazwy metod

- **Setter i getter mają tę samą nazwę.** Setter przyjmuje wartość i zwraca obiekt
  (`Size(v)`), getter nie ma argumentów i zwraca wartość (`Size()`).
- **Getter stanu logicznego ma prefiks `Is`:** `IsFullscreen()`, `IsHovered()`,
  `IsIncremental()`. Setter takiej flagi to sama nazwa z domyślnym `true`:
  `Fullscreen()`, `Incremental()`, `Fill()`.
- **Zapytania z argumentem nie mają prefiksu:** `KeyDown(key)`, `FileExists(path)`,
  `Contains(point)`.
- **Bez prefiksu `Get`.** Wyjątek to `Engine::Get()` (dostęp do singletonu). Obiekty podrzędne
  pobiera się metodą nazwaną jak obiekt: `engine.Scene("menu")`, `engine.Input()`,
  `scene.Camera()`, `Context<T>()`.
- **Zdarzenia zaczynają się od `On`:** `OnClick()`, `OnUpdate()`, `OnDraw()`.

## Łańcuchowanie

Każda metoda konfiguracji zwraca obiekt, na którym ją wywołano. W klasach z hierarchii Entity
(kształty, `ui::TextField`, klasy bazowe) robi to deducing this:

```cpp
template <typename Self> Self& Radius(this Self& self, float radius) {
    self.radius_ = radius;
    return self;
}
```

Dzięki temu łańcuch zachowuje typ także w klasie pochodnej
(`class Ball : public shapes::Circle`). Klasy spoza hierarchii (`Engine`, `Scene`,
`SceneCamera`, `PrimitiveRenderer`, `Input`) zwracają po prostu `*this`.

Callbacki zdarzeń dostają obiekt jako pierwszy argument (`Callback<Self, ...>`). Przechwycenie
w lambdzie nazywamy `fn`, nie tak samo jak parametr (projekt kompiluje się z `-Wshadow`).

## Typy

- **Punkt to `Point2D`.** Pozycje, środki, wierzchołki i ziarna wypełnień w `PrimitiveRenderer`
  i w kształtach z `shapes` są typu `Point2D`; listy wierzchołków to `std::vector<Point2D>`.
  Pojedynczy `Vector2` albo `{x, y}` konwertuje się na `Point2D` niejawnie.
- **Rozmiar, promień i przesunięcie to `Vector2`**, bo to wektory, a nie punkty.
- **API sceny i wejścia używa `Vector2`** (`Position()`, `Translate()`, `Contains()`,
  `MousePosition()`), bo wartości pochodzą wprost z raylib.
- **Kąty są w radianach** wszędzie w API; przeliczenie na stopnie dla raylib jest w jednym
  miejscu (`SceneCamera::Rotation()`).

## Obsługa błędów (Engine)

- Konfiguracja i polecenia zwracają `Engine&`, a niepoprawne wywołanie tylko zapisują w logu
  (`LogLevel::Warning` albo `LogLevel::Error`), żeby nie przerywać łańcucha.
- Operacje na plikach i nagraniach zwracają wynik: `bool` albo `std::optional`.
- `Init()` loguje błąd, a wynik podaje `IsReady()`.
- Wyjątek rzuca tylko `Engine::Scene()`, bo musi zwrócić referencję do sceny, której nie ma.

## Wywołania raylib

Funkcje raylib wołamy z prefiksem `::` (np. `::DrawText`), żeby nie myliły się z metodami
silnika o podobnych nazwach (`Engine::IsWindowFocused()` i `::IsWindowFocused()`).
Odczyt wejścia w silniku przechodzi wyłącznie przez klasę `Input`.

## Język

- Identyfikatory są po angielsku.
- Komentarze i dokumentacja Doxygen są po polsku, z polskimi znakami.
- Tekst wyświetlany w oknie i zapisywany w logu jest po polsku **bez** polskich znaków:
  domyślny font raylib nie ma tych glifów, a konsola Windows bez `/utf-8` pokazałaby krzaki.
- Nazwy scen są po polsku (`"prymitywy"`), nazwy plików po angielsku (`PrimitivesScene.cpp`).

## Logi

`Engine::Log(LogLevel, tekst)`. Każdy wpis zaczyna się znacznikiem `[DEBUG]`, `[INFO]`,
`[UWAGA]` albo `[BLAD]`; komunikaty raylib mają dodatkowo `[raylib]`. `Log(tekst)` bez
poziomu oznacza `LogLevel::Info`.
