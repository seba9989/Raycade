# Konwencje kodu Raycade {#konwencje}

Zasady, których trzyma się API silnika. Nowy kod (także sceny gry) powinien je stosować,
żeby całość czytała się jednakowo.

## Układ plików

Nagłówki leżą w `engine/include/raycade/` w katalogach-warstwach. Każda warstwa dołącza tylko
warstwy wcześniejsze na liście:

| Katalog      | Zawartość                                                       | Może dołączać               |
|--------------|-----------------------------------------------------------------|-----------------------------|
| `math/`      | AffineMatrix2D, Point2D, LineSegment, geometria, ConvexPart (części kształtów kolizji) | - |
| `render/`    | Bitmap, Assets, RenderTarget, ClipStack, ShaderProgram, Typeface, PrimitiveRenderer | math    |
| `input/`     | Input, akcje i osie                                             | -                           |
| `entity/`    | Entity, Group i klasy bazowe (Transformable, Drawable, Updatable...) | math, render, input    |
| `shapes/`    | figury geometryczne z przestrzeni nazw `shapes` (baza `shapes::Shape`) | entity i jej zależności |
| `visual/`    | obrazy, tekst i grupy rysowane do tekstury z przestrzeni nazw `visual` | entity i jej zależności |
| `ui/`        | elementy interfejsu z przestrzeni nazw `ui`                     | entity i jej zależności     |
| `physics/`   | `Collider<S>`, StaticBody, CharacterBody, RigidBody, Area, PhysicsWorld (Box2D) | shapes, visual i ich zależności |
| `scene/`     | Scene, SceneCamera, PointerSystem (trafienia myszą)             | physics i jej zależności (Scene ma PhysicsWorld) |
| `engine/EngineTypes.hpp` | typy ustawień: LogLevel, Scaling                    | -                           |
| `Engine.hpp` | pętla, okno, sceny, zasoby                                      | wszystkie powyższe          |
| `engine/SceneMacro.hpp` | makro SCENE (wymaga pełnej klasy Engine)             | `Engine.hpp`                |

Nagłówek zbiorczy katalogu leży obok niego (`shapes.hpp` obok `shapes/`, `visual.hpp` obok
`visual/`, `physics.hpp` obok `physics/`, `ui.hpp` obok `ui/`). `<raycade>` (w `engine/umbrella/`)
dołącza `Engine.hpp`, `engine/SceneMacro.hpp`, `entity/Group.hpp`, `shapes.hpp`, `visual.hpp`,
`physics.hpp` i `ui.hpp`. Źródła w `engine/src/` odwzorowują ten sam układ. Implementacja
`Engine` jest w `src/engine/`, podzielona tematycznie na pliki `Engine*.cpp` - który plik
zawiera którą grupę metod, opisuje tabela w dokumentacji `Engine.hpp`. Nagłówki prywatne
implementacji (np. `src/render/primitive_renderer/Pixel.hpp`) zostają w `src/`.

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
  `IsFilled()`. Setter takiej flagi to sama nazwa z domyślnym `true`:
  `Fullscreen()`, `Fill()`.
- **Wybór jednej z kilku wartości to setter z typem wyliczeniowym**, a nie kilka flag:
  `Raster(Rasterizer::Custom)`, `ScaleMode(Scaling::Fit)`. Typ ma inną nazwę niż metoda
  (`Rasterizer`, nie `Raster`), bo wewnątrz klasy nazwa metody zasłoniłaby typ.
- **Zapytania z argumentem nie mają prefiksu:** `KeyDown(key)`, `FileExists(path)`,
  `Contains(point)`. Wyjątek: zapytanie o flagę stanu ma prefiks `Is`, tak jak w raylib -
  `Engine::IsWindowState(FLAG_...)` czyta się jak pytanie o stan, a nie o zdarzenie.
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

Pola i prywatne metody klasy czyta się wprost przez `self` (`self.radius_`,
`self.SheetChanged()` w `visual::Sprite`), bez rzutowania na klasę bazową - dostęp jest sprawdzany w klasie,
która definiuje metodę, więc działa też dla prywatnych składowych.

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
- **Wymiary okna i monitora w pikselach są `int`**, jak w raylib: `Window(w, h)`,
  `Resolution(w, h)`, `MinSize()`, `MaxSize()`, `ScreenWidth()`. Pozycje (okna, kursora) są
  `Vector2` - setter przyjmuje ten sam typ, który zwraca getter (`WindowPosition()`).
- **API sceny i wejścia używa `Vector2`** (`Position()`, `Translate()`, `Contains()`,
  `MousePosition()`), bo wartości pochodzą wprost z raylib.
- **Kąty są w radianach** wszędzie w API; przeliczenie ze stopni raylib jest tylko tam, gdzie
  silnik opakowuje wartość raylib (`SceneCamera::Rotation()`, `Input::GestureDragAngle()`,
  `Input::GesturePinchAngle()`).

## Zasoby

Zasoby wczytywane z pliku albo kompilowane (Bitmap, Typeface, ShaderProgram) mają statyczne
`Load()` zwracające `std::optional` - brak wartości oznacza błąd, a jego przyczynę zapisuje
w logu raylib. Obiekty sceny przyjmują zasoby jako `std::shared_ptr` (wiele obiektów może
używać jednej tekstury czy shadera); zamianę robi `Shared()`:

```cpp
auto wave = Shared(ShaderProgram::Load("", "assets/wave.fs")); // nullptr przy błędzie
```

Zasoby tylko do odczytu (Bitmap, Typeface) przekazuje się jako `std::shared_ptr<const T>`,
a ShaderProgram bez `const`, bo ustawia się w nim uniformy.

Bitmapy i czcionki z plików gry pobiera się przez pamięć podręczną `Engine::Assets()`, a nie
przez `Load()` w konfiguracji sceny. Nazwa jest liczona od katalogu `assets/`, ten sam plik
wczytuje się raz, a brakujący daje zasób zastępczy (szachownica, font raylib) i jeden wpis
w logu zamiast `nullptr`:

```cpp
auto hero = engine.Assets().Bitmap("hero.png");
auto font = engine.Assets().Font("font.ttf");
auto anim = engine.Assets().Sheet("hero.json"); // Aseprite / TexturePacker
```

Arkusze sprite'ów z programów graficznych (eksport JSON z Aseprite albo TexturePackera) wczytuje
`Assets::Sheet()`: klatki, czasy klatek i animacje z tagów, a obraz z `meta.image` trafia do tej
samej pamięci podręcznej co `Bitmap()`. Zwrócony arkusz jest tylko do odczytu - animacje
i zdarzenia klatek dopisuje się do kopii (`std::make_shared<SpriteSheet>(*anim)`).

`Load()` zostaje dla zasobów spoza gry (np. plik wskazany przez użytkownika) i dla kodu, który
sam chce obsłużyć błąd. `ShaderProgram` nie przechodzi przez `Assets`, bo jego uniformy są
stanem obiektu, który go używa.

## Obsługa błędów (Engine)

- Konfiguracja i polecenia zwracają `Engine&`, a niepoprawne wywołanie tylko zapisują w logu
  (`LogLevel::Warning` albo `LogLevel::Error`), żeby nie przerywać łańcucha.
- Operacje na plikach i nagraniach zwracają wynik: `bool` albo `std::optional`.
  Operacje na plikach (`LoadText()`, `SaveData()`, `ListDirectory()`, `ChangeDirectory()`...)
  są `const`, bo nie zmieniają stanu silnika, i nie dopisują własnego wpisu o błędzie -
  przyczynę zapisuje w logu raylib (`[raylib] FILEIO: ...`). Nagrania (`StopRecording()`,
  `Replay()`) zmieniają stan silnika i same zapisują wynik w logu.
- `Init()` loguje błąd, a wynik podaje `IsReady()`.
- Wyjątek rzuca tylko `Engine::Scene()`, bo musi zwrócić referencję do sceny, której nie ma.

## Pliki i ścieżki

Odczyt i zapis plików ma silnik: `LoadText()`/`SaveText()`, `LoadData()`/`SaveData()`
(bajty), `ListDirectory()`, `MakeDirectory()`. Rozbiór ścieżek (nazwa pliku, rozszerzenie,
katalog nadrzędny, łączenie) robi `std::filesystem::path` z biblioteki standardowej - silnik
celowo nie opakowuje `GetFileName()`, `GetFileExtension()` i podobnych funkcji raylib, które
zwracają wskaźniki do wspólnego bufora nadpisywanego przy kolejnym wywołaniu.

## Wywołania raylib

Funkcje raylib wołamy z prefiksem `::` (np. `::DrawText`), żeby nie myliły się z metodami
silnika o podobnych nazwach (`Engine::IsWindowFocused()` i `::IsWindowFocused()`).
Odczyt wejścia w silniku przechodzi wyłącznie przez klasę `Input`.

## Język

- Identyfikatory są po angielsku.
- Komentarze i dokumentacja Doxygen są po polsku, z polskimi znakami.
- Tekst zapisywany w logu jest po polsku **bez** polskich znaków: konsola Windows bez
  `/utf-8` pokazałaby krzaki. Tekst w oknie też, chyba że jest rysowany czcionką wczytaną
  przez `Typeface::Load()` (ma polskie litery) - domyślny font raylib ich nie ma. Napis
  rysowany taką czcionką ma polskie litery, a napis widoczny tylko bez niej (np. komunikat
  o braku pliku czcionki) - nie. Nazwy akcji (`BindAction("nowa pilka", ...)`) to
  identyfikatory, więc też są bez polskich znaków.
- Nazwy scen są po polsku (`"prymitywy"`), nazwy plików po angielsku (`PrimitivesScene.cpp`).

## Logi

`Engine::Log(LogLevel, tekst)`. Każdy wpis zaczyna się znacznikiem `[DEBUG]`, `[INFO]`,
`[UWAGA]` albo `[BLAD]`; komunikaty raylib mają dodatkowo `[raylib]`. `Log(tekst)` bez
poziomu oznacza `LogLevel::Info`.
