# Raycade

Silnik 2D w C++23 na [raylib](https://www.raylib.com/), z API opartym na kaskadzie wywołań.
Nazwa: **ray**lib + cas**cade** (łańcuch metod) + **arcade**.

```cpp
#include <raycade>

SCENE("menu") {
    scene.Spawn<shapes::Circle>()
        .Position({400, 300})
        .Radius(20)
        .Paint(ORANGE)
        .Fill()
        .OnClick([](shapes::Circle& self) { self.Paint(RED); });
}

int main() { return Engine::Get().StartScene("menu").Run(); }
```

Projekt na zajęcia z Podstaw Grafiki Komputerowej. Program `demo/` pokazuje funkcje
z laboratoriów: własne algorytmy odcinka, okręgu i elipsy, wypełnianie scanline, flood fill
i boundary fill, przekształcenia, hierarchię obiektów, kamerę, obsługę wejścia, animację
bitmap, przycinanie, shadery i krok stały.

## Wymagania

- CMake 3.24+
- kompilator z C++23 (deducing this): Clang 18+, GCC 14+ albo MSVC 19.32+
- raylib 5.x - jeśli nie ma go w systemie, CMake pobiera wersję 5.5

## Kompilacja

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/raycade-demo
```

Z [devenv](https://devenv.sh/): `build`, `run`, `fmt`, `docs`, `clean`.

Klawisze 1-8 przełączają sceny demo, F11 - pełny ekran. Opis każdej sceny jest na stronie
głównej dokumentacji.

## Użycie we własnym projekcie

```cmake
add_subdirectory(raycade)   # demo budowane jest tylko w głównym projekcie
target_link_libraries(gra PRIVATE raycade::raycade)
```

Gra dołącza jeden nagłówek: `#include <raycade>`. Sceny (makro `SCENE`) muszą leżeć
w plikach programu, nie w bibliotece.

## Struktura

```
engine/
  umbrella/raycade        nagłówek zbiorczy <raycade>
  include/raycade/        nagłówki publiczne, w warstwach:
    math/                   macierz przekształceń, Point2D, geometria
    render/                 Bitmap, RenderTarget, PrimitiveRenderer
    input/                  klawiatura, mysz, pad, akcje i osie
    entity/                 Entity i klasy bazowe
    shapes/  shapes.hpp     kształty (namespace shapes)
    ui/      ui.hpp         interfejs (namespace ui)
    scene/                  Scene, SceneCamera
    engine/                 typy ustawień silnika, makro SCENE
    Engine.hpp              okno, pętla, sceny
  src/                    implementacja w tym samym układzie
    engine/                 Engine podzielony na pliki: okno, sceny, wejście, pliki...
demo/                     program demonstracyjny i sceny laboratoriów
docs/                     Doxyfile, KONWENCJE.md
```

## Dokumentacja

```sh
doxygen docs/Doxyfile     # albo: docs (devenv)
```

Wynik trafia do `docs/html/index.html`. Zasady nazewnictwa, typów i układu plików
opisuje [docs/KONWENCJE.md](docs/KONWENCJE.md).
