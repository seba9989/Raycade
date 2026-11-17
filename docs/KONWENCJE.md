# Konwencje kodu Raycade {#konwencje}

Zasady, których trzyma się API silnika. Nowy kod (także sceny gry) powinien je stosować,
żeby całość czytała się jednakowo.

## Układ plików

Nagłówki leżą w `engine/include/raycade/` w katalogach-warstwach. Każda warstwa dołącza tylko
warstwy wcześniejsze na liście:

| Katalog      | Zawartość                                                       | Może dołączać               |
|--------------|-----------------------------------------------------------------|-----------------------------|
| `core/`      | Shared (zasób w `shared_ptr`), EngineTypes (LogLevel, Scaling)  | -                           |
| `math/`      | `transform/` AffineMatrix2D; `geometry/` Point2D, LineSegment, geometria, ConvexPart (części kształtów kolizji) | - |
| `assets/`    | Assets (pamięć podręczna); `image/` Bitmap, SpriteSheet; `font/` Typeface; `shader/` ShaderProgram; `sound/` AudioClip | core |
| `render/`    | RenderTarget, ClipStack; `primitives/` PrimitiveRenderer        | assets, math                |
| `input/`     | Input, akcje i osie                                             | -                           |
| `audio/`     | Audio (fasada) i z przestrzeni nazw `audio`: `playback/` Source, Sound, Music, Voice; `mixing/` Bus (raylib audio) | assets |
| `entity/`    | Entity; `traits/` klasy bazowe (Transformable, Drawable, Updatable, Focusable, Contextable) | math, assets, render, input |
| `node/Group.hpp` | `node::Group` - pusty węzeł grupujący                       | entity i jej zależności     |
| `node/shapes/` | figury z przestrzeni nazw `node::shapes` (baza `Shape`); `point/`, `line/`, `polygon/`, `round/` | entity i jej zależności |
| `node/visual/` | z przestrzeni nazw `node::visual`: `image/` Sprite, Canvas; `text/` Text; `group/` RenderGroup | entity i jej zależności |
| `node/ui/`   | elementy interfejsu z przestrzeni nazw `node::ui`               | entity i jej zależności     |
| `node/sound/` | z przestrzeni nazw `node::sound`: Emitter, Listener           | audio, entity               |
| `node/collider/` | z przestrzeni nazw `node::collider`: `Collider<S>`, CollisionShape, aliasy (`Rect`, `Circle`...) | node/shapes, node/visual    |
| `node/physics/` | z przestrzeni nazw `node::physics`: CollisionObject, PhysicsBody; `body/` StaticBody, CharacterBody, RigidBody; `area/` Area | node/collider i jej zależności |
| `scene/`     | Scene; `camera/` SceneCamera; `pointer/` PointerSystem (trafienia myszą); `physics/` PhysicsWorld (Box2D) | node/physics i jej zależności (Scene ma PhysicsWorld) |
| `engine/Engine.hpp` | pętla, okno, sceny, zasoby, dźwięk                       | wszystkie powyższe          |
| `engine/SceneMacro.hpp` | makro SCENE (wymaga pełnej klasy Engine)             | `engine/Engine.hpp`         |

W `node/` i w przestrzeni nazw `node` leży wszystko, co gra tworzy przez `Spawn<T>()` (patrz
sekcja „Przestrzenie nazw”). Każda grupa tematyczna ma własny podkatalog, także gdy
ma jeden plik (`assets/font/`, `scene/camera/`). Podkatalog nigdy nie jest niższą warstwą niż
jego rodzic. Przestrzeń nazw kończy się na drugim poziomie: `node/shapes/round/Circle.hpp` to
`node::shapes::Circle` - głębsze katalogi porządkują tylko pliki. Jeden plik zawiera jedną klasę; małe struktury pomocnicze (`Collision`,
`RayHit`, `AreaShapeContact`) leżą przy klasie, której dotyczą.

Nagłówki zbiorcze leżą obok katalogu przestrzeni nazw: `audio.hpp` obok `audio/` (mikser),
`node/shapes.hpp`, `node/visual.hpp`, `node/ui.hpp`, `node/sound.hpp`, `node/collider.hpp`
i `node/physics.hpp` (dołącza też collidery), a `node.hpp` zbiera je wszystkie razem z `Group`.
Podkatalogi tematyczne (`round/`, `body/`) nie mają własnych. `<raycade>` (w `engine/umbrella/`)
dołącza `engine/Engine.hpp`, `engine/SceneMacro.hpp`, `audio.hpp` i `node.hpp`.

Źródła w `engine/src/` odwzorowują ten sam układ 1:1. Nagłówki prywatne implementacji leżą
obok swoich `.cpp`: mikser dźwięku `src/audio/AudioMixer.hpp` (gra korzysta z fasady
`Engine::Audio()` i uchwytów `audio::Voice`), `src/render/primitives/Pixel.hpp`,
`src/node/shapes/HitDistance.hpp`. Implementacja `Engine` jest w `src/engine/`, podzielona
tematycznie na pliki `Engine*.cpp` - który plik zawiera którą grupę metod, opisuje tabela
w dokumentacji `engine/Engine.hpp`.

W nagłówku zostaje tylko to, co musi lub powinno być inline: szablony (settery z deducing
this, `Spawn<T>()`, `Follow<T>()`), jedno- i dwuwierszowe gettery/settery oraz drobna
matematyka (`AffineMatrix2D`). Pozostałe ciała metod - w tym wirtualne `Draw()`, `Contains()`
i `Tick()` - są w odpowiadającym pliku `.cpp`. Typ potrzebny tylko w implementacji
deklaruje się zapowiadająco (`class SceneCamera;`), żeby nie łamać kolejności warstw.

Wewnątrz silnika nagłówki dołącza się zawsze pełną ścieżką: `#include <raycade/entity/Entity.hpp>`,
nigdy względną (`"../Entity.hpp"`). Ścieżka względna zostaje tylko dla nagłówków prywatnych
w `src/`. Katalogi mają nazwy małymi literami, pliki z klasami - PascalCase.

## Przestrzenie nazw

Wszystko, co gra tworzy przez `Spawn<T>()`, jest w przestrzeni nazw `node`, podzielonej
tematycznie - wpisanie `Spawn<node::` podpowiada kategorie, a `Spawn<node::shapes::` typy:

| Przestrzeń nazw  | Typy                                                                  |
|------------------|-----------------------------------------------------------------------|
| `node`           | `Group`                                                               |
| `node::shapes`   | figury (`Rect`, `Circle`, `Polygon`...) i ich baza `Shape`            |
| `node::visual`   | `Sprite`, `Text`, `Canvas`, `RenderGroup`                             |
| `node::ui`       | `TextField`                                                           |
| `node::physics`  | `StaticBody`, `CharacterBody`, `RigidBody`, `Area`, bazy `PhysicsBody` i `CollisionObject`, `Collision` |
| `node::collider` | `Collider<S>`, `CollisionShape`, aliasy (`collider::Rect` = `Collider<shapes::Rect>`) |
| `node::sound`    | `Emitter`, `Listener`                                                 |

- **Klasy bazowe drzewa są poza `node`** (`Entity`, `Transformable`, `Drawable`, `Updatable`,
  `Focusable`, `Contextable`) - nie tworzy się ich wprost, a dziedziczą po nich także obiekty
  gry. Bazę w `node` ma tylko kategoria, w której jest wspólnym typem do przechowywania
  (`shapes::Shape`, `physics::PhysicsBody`).
- **Poza `node` są też Engine, Scene, SceneCamera, PhysicsWorld, zasoby i mikser** - to nie są
  obiekty drzewa sceny. Mikser ma własną przestrzeń `audio` (patrz sekcja „Dźwięk”), dlatego
  obiekty sceny dźwięku to `node::sound`, a nie `node::audio`.
- **Przestrzeń nazw ma dwa poziomy**: `node::kategoria::Typ`. Głębsze katalogi
  (`node/shapes/round/`) nie dodają poziomów.
- Gra może napisać `using namespace node;` i używać `shapes::Circle`, `physics::StaticBody`;
  `audio::` nadal oznacza mikser. Kod silnika poza `node` pisze pełne nazwy w nagłówkach,
  a w plikach `.cpp` może użyć deklaracji `using node::physics::RigidBody;` dla konkretnych typów.

## Nazwy metod

- **Setter i getter mają tę samą nazwę.** Setter przyjmuje wartość i zwraca obiekt
  (`Size(v)`), getter nie ma argumentów i zwraca wartość (`Size()`).
- **Getter stanu logicznego ma prefiks `Is`:** `IsFullscreen()`, `IsHovered()`,
  `IsFilled()`. Setter takiej flagi to **dokładnie ta sama nazwa bez `Is`**, z domyślnym
  `true`: `Fullscreen()`, `Filled()`, `FlippedX()`, `RotationLocked()`, `CursorHidden()`,
  `KeyboardEnabled()`. Nazwa jest więc przymiotnikiem albo imiesłowem opisującym stan, a nie
  czasownikiem (`Filled()`, nie `Fill()`; `CursorHidden()`, nie `HideCursor()`).
- **Wybór jednej z kilku wartości to setter z typem wyliczeniowym**, a nie kilka flag:
  `Raster(Rasterizer::Custom)`, `ScaleMode(Scaling::Fit)`. Typ ma inną nazwę niż metoda
  (`Rasterizer`, nie `Raster`), bo wewnątrz klasy nazwa metody zasłoniłaby typ.
- **Zapytania z argumentem nie mają prefiksu:** `KeyDown(key)`, `FileExists(path)`,
  `Contains(point)`. Wyjątek: zapytanie o flagę stanu ma prefiks `Is`, tak jak w raylib -
  `Engine::IsWindowState(FLAG_...)` czyta się jak pytanie o stan, a nie o zdarzenie.
- **Bez prefiksu `Get`.** Wyjątek to `Engine::Get()` (dostęp do singletonu). Obiekty podrzędne
  pobiera się metodą nazwaną jak obiekt: `engine.Scene("menu")`, `engine.Input()`,
  `scene.Camera()`, `Context<T>()`.
- **Rzeczownik ustawia wartość, czasownik ją zmienia.** `Position(p)`, `Rotation(r)`,
  `Scale(s)` ustawiają wartość bezwzględną (i mają gettery), a `Translate(v)`, `Rotate(r)`,
  `ScaleBy(s)` dodają albo mnożą bieżącą. Metoda, która dopisuje element do kolekcji, ma
  prefiks `Add` (`SpriteSheet::AddFrame()`), żeby nie wyglądała jak setter.
- **Jedna nazwa - jedno znaczenie.** Ta sama nazwa nie oznacza dwóch różnych rzeczy w różnych
  klasach: przycięcie rysunku to `ClipRect()`, animacja to `Animation()` (w `node::visual::Sprite`
  i w `SpriteSheet`), zdarzenie puszczenia przycisku to `OnMouseReleased()` - jak zapytanie
  `Input::MouseReleased()`, a nie stan `Input::MouseUp()`.
- **Punkt zaczepienia** obiektów z rozmiarem (`node::visual::Sprite`, `node::visual::Text`) to
  `Origin()` w pikselach plus `Anchor()` jako część rozmiaru; wynik podaje `Pivot()`.
- **Zdarzenia zaczynają się od `On`:** `OnClick()`, `OnUpdate()`, `OnDraw()`. Ponowne
  wywołanie zastępuje poprzednią reakcję (w `OnKeyPressed(key, ...)` - reakcję na ten sam
  klawisz). Pary zdarzeń dostają ten sam typ argumentu: `OnCollisionEnter/Stay/Exit()`
  dostają `const node::physics::Collision&`.
- **Jedno pojęcie - jedna nazwa:** prędkość ciała to `Velocity()` (w `CharacterBody`
  i `RigidBody`), liczba elementów to `...Count()` (`FrameCount()`, `SheetFrameCount()`),
  zasób z `Load()` sprawdza się przez `IsLoaded()`, a obiekt GPU bez pliku
  (`RenderTarget`) przez `IsValid()`.
- **Numer urządzenia jest ostatnim parametrem z domyślnym 0:** `GamepadDown(button, gamepad)`,
  `GamepadAxis(axis, gamepad)`, `bind::Pad(button, gamepad)`, `TouchPosition(index)`.
- **Wyszukiwanie (`Find...`) zwraca brak wartości zamiast liczby magicznej:** wskaźnik
  (`FindAnimation()`, `Context<T>()`) albo `std::optional` (`FindFrame()`).
- **Obiekty, które przyjmują zasób, przyjmują też nazwę pliku:** `Sprite::Sheet("hero.png")`,
  `Text::Font("font.ttf")`, `Drawable::Shader("glow.fs")` - z `Assets` w kontekście, a bez niego
  z `Assets::Local()` (nazwa względem katalogu roboczego, ten sam zasób zastępczy i wpis w logu).

## Bez duplikatów

Każda funkcja ma jedno miejsce. `Engine` nie dubluje metod `Input` (`engine.Input().KeyDown()`)
ani `Clear()` bitmap i tekstur; sterowanie oknem i kursorem jest w `Engine`, a odczyt wejścia
w `Input`. Metody, które prowadzą wewnętrzny przebieg (kroki `PhysicsWorld`,
`SceneCamera::Update()`), są prywatne i dostępne tylko dla klas, które je wywołują (`friend`).

## Łańcuchowanie

Każda metoda konfiguracji zwraca obiekt, na którym ją wywołano. W klasach z hierarchii Entity
(kształty, `node::ui::TextField`, klasy bazowe) robi to deducing this:

```cpp
template <typename Self> Self& Radius(this Self& self, float radius) {
    self.radius_ = radius;
    return self;
}
```

Pola i prywatne metody klasy czyta się wprost przez `self` (`self.radius_`,
`self.SheetChanged()` w `node::visual::Sprite`), bez rzutowania na klasę bazową - dostęp jest sprawdzany w klasie,
która definiuje metodę, więc działa też dla prywatnych składowych.

Dzięki temu łańcuch zachowuje typ także w klasie pochodnej
(`class Ball : public node::shapes::Circle`). Klasy spoza hierarchii (`Engine`, `Scene`,
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

Każdy zasób z pliku gry - bitmapę, arkusz, czcionkę, shader i dane dźwięku - pobiera się przez
pamięć podręczną `Engine::Assets()`, a nie przez `Load()` w konfiguracji sceny. Wszystkie typy
mają ten sam kontrakt:

- nazwa jest liczona od katalogu `assets/` (`Assets::Path()`, to samo co `Engine::AssetPath()`);
  ścieżka bezwzględna zostaje bez zmian,
- ten sam plik wczytuje się raz - kluczem jest znormalizowana ścieżka,
- brakujący plik daje zasób zastępczy zamiast `nullptr` (szachownica, font raylib, pusty shader,
  cisza) i jeden wpis `[UWAGA]` w logu na plik, nawet gdy gra prosi o niego co klatkę,
- czy plik się wczytał, mówi `IsLoaded()` - ma ją każdy typ zasobu,
- `Purge()` po zwolnieniu sceny usuwa zasoby, których nie trzyma już nikt poza pamięcią.

```cpp
auto hero = engine.Assets().Bitmap("hero.png");
auto anim = engine.Assets().Sheet("hero.json"); // Aseprite / TexturePacker
auto font = engine.Assets().Font("font.ttf");
auto wave = engine.Assets().Shader("wave.fs");  // albo Shader("wave.vs", "wave.fs")
auto roar = engine.Assets().Clip("roar.wav");   // dane pliku dźwięku (AudioClip)
```

Zasoby tylko do odczytu (Bitmap, SpriteSheet, Typeface, AudioClip) są zwracane jako
`std::shared_ptr<const T>`, a ShaderProgram bez `const`, bo ustawia się w nim uniformy. Shader
z `Assets` jest wspólny dla wszystkich, którzy pobrali ten plik - razem z uniformami. Shader
wymaga otwartego okna (np. konfiguracji sceny); wcześniej `Assets` zwraca pusty.

Arkusze sprite'ów (eksport JSON z Aseprite albo TexturePackera) wczytuje `Assets::Sheet()`:
klatki, czasy klatek i animacje z tagów, a obraz z `meta.image` trafia do tej samej pamięci
podręcznej co `Bitmap()`. Arkusz jest tylko do odczytu - animacje i zdarzenia klatek dopisuje
się do kopii (`std::make_shared<SpriteSheet>(*anim)`).

`Assets` trzyma tylko dane plików. Ustawienia dźwięków (`engine.Audio().Sound(name)`) są
w mikserze, bo `Purge()` zgubiłby konfigurację z `main()`, a nazwa dźwięku nie musi być plikiem
(patrz „Dźwięk”). Żeby pierwsze odtworzenie nie czekało na dysk, gra trzyma wynik `Clip()`.

Obiekty sceny przyjmują zasoby jako `std::shared_ptr` albo nazwę pliku (patrz „Nazwy metod”).
Każdy typ ma też statyczne `Load()` zwracające `std::optional` - brak wartości oznacza błąd,
a jego przyczynę zapisuje log raylib. Zostaje ono dla zasobów spoza gry (np. pliku wskazanego
przez użytkownika), dla kodu, który sam chce obsłużyć błąd, i dla shadera z własnymi
uniformami dla jednego obiektu; zamianę na wskaźnik robi `Shared()`:

```cpp
auto wave = Shared(ShaderProgram::Load("", "assets/wave.fs")); // nullptr przy błędzie
```

Nowy typ zasobu z pliku dostaje metodę w `Assets` i ten sam kontrakt.

## Dźwięk

Dźwięk ma jeden punkt wejścia, jak zasoby: `engine.Audio()` zwraca fasadę `Audio`, a z niej
pobiera się po nazwie dźwięki (`Sound(name)`), muzykę (`Music(name)`) i szyny (`Bus(name)`).
Pierwsze pobranie tworzy obiekt z ustawieniami domyślnymi, a kolejne zwracają ten sam, więc
konfiguruje się go raz (np. w `main()`), a odtwarza w dowolnym miejscu. Ustawienia mnożą się
w górę: głos (`audio::Voice`) - dźwięk albo muzyka (`audio::Sound`, `audio::Music`) - szyny
(`audio::Bus`) aż do szyny głównej, której ustawienia ma sama fasada (`engine.Audio().Volume()`).

```cpp
Audio& audio = engine.Audio();
audio.Bus("kroki").Parent("efekty").Volume(0.6f);
audio.Sound("kroki").Files({"step1.wav", "step2.wav"}).PitchVariation(0.05f).Bus("kroki");
audio.Music("theme.ogg").FadeInTime(1.5f).FadeOutTime(1.0f);

audio.Sound("kroki").Play();                 // Play() tylko odtwarza
audio.Sound("coin.wav").Play().Pitch(1.5f);  // zmiana jednego odtworzenia
audio.Music("theme.ogg").Play();             // poprzednia ścieżka się wycisza
audio.Bus("efekty").Paused();                // stan całej grupy
```

- **Typy dźwięku są w przestrzeni nazw `audio`**, bo raylib ma globalne `Sound` i `Music`.
  Z tego samego powodu w grze nie używa się `using namespace audio;`.
- **Dźwięk (`audio::Sound`) i muzyka (`audio::Music`) to osobne typy** ze wspólną bazą
  `audio::Source` (pliki, głośność, szyna, pętla, wyciszanie). Dźwięk gra wiele razy naraz
  i ma limity (`MaxVoices()`, `MinInterval()`); muzyka gra jednym odtworzeniem, zawsze jako
  strumień, domyślnie w pętli na szynie `"music"` (`Audio::kMusicBus`, z `Exclusive()`),
  a pauzę, przewijanie i `OnFinish()` ma na sobie. Nazwy dźwięków i muzyki są osobne.

- **Nazwa dźwięku jest plikiem** (względem `assets/`), chyba że `Files()` poda listę wariantów -
  jedna lista jednym setterem, jak `Polygon::Vertices()`.
- **`Sound::Play()` zwraca uchwyt `audio::Voice`**, a nie dźwięk - tworzy nowe odtworzenie, tak jak `Spawn<T>()`
  zwraca nowy obiekt. Głos rusza na końcu klatki, więc ustawienia z łańcucha działają od pierwszej
  próbki. Uchwyt skończonego głosu jest martwy i ignoruje polecenia bez wpisu w logu.
- **Szyna to grupa dźwięków** (`audio::Bus`), a nie kanał - „kanał” w audio znaczy też kanał
  stereo albo pojedynczy głos. Setter dźwięku nazywa się krótko `Bus()`.
- **Polecenia odtwarzania to czasowniki** (`Play()`, `Stop()`), jak w `node::visual::Sprite`, a trwały
  stan grupy to flagi szyny (`Paused()`, `Muted()`).
- **Dźwięk w miejscu świata gra przez `node::sound::Emitter`** - głos nie ma położenia, żeby nie
  dublować `Transformable`. Słuchacz to `node::sound::Listener` (bez niego - środek kamery sceny).
  Obiekty sceny są w `node::sound`, a nie `node::audio`, żeby `using namespace node;` nie robiło
  z `audio::` nazwy niejednoznacznej.
- **Głos należy do sceny**, w której aktualizacji albo konfiguracji go zagrano - przy
  `UnloadScene()`/`ReloadScene()` cichnie (bez `OnFinish()`), jak węzeł dźwięku w drzewie
  sceny. Grają dalej głosy z `Global()` (jak obiekt globalny; muzyka ma je domyślnie) i głosy
  zagrane z globalnego `Engine::OnUpdate()` albo z `main()`. Szyna nie zmienia tej reguły.
- Nazwy szyn to identyfikatory - bez polskich znaków (`"muzyka"`, `"efekty"`).

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
  `Audio::Sound()`, `Audio::Music()` i `Audio::Bus()` też zwracają referencję, ale nieznaną nazwę tworzą
  (jak `Assets` - brakujący plik gra ciszą z jednym wpisem w logu).

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
