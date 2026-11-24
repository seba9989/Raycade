cmake_minimum_required(VERSION 3.24)
project(raycade LANGUAGES C CXX)

# Wymagany standard (C++23, deducing this) ustawia sama biblioteka przez
# target_compile_features - wystarczy GCC 14+, Clang 18+ albo MSVC z VS 2022 17.2+.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Demo budujemy tylko wtedy, gdy ten katalog jest głównym projektem. Inny projekt
# (np. silnik 3D) może dołączyć bibliotekę przez add_subdirectory() bez scen demo.
option(RAYCADE_BUILD_DEMO "Buduj program demonstracyjny" ${PROJECT_IS_TOP_LEVEL})

# Zależności w ściśle określonych wersjach, zawsze pobierane przez FetchContent - niezależnie od
# tego, co jest zainstalowane w systemie. Archiwa są sprawdzane sumą SHA256, więc każdy (devenv,
# Windows u prowadzącego, CI) kompiluje dokładnie ten sam kod. Zmiana wersji = zmiana adresu i
# sumy poniżej (sha256sum pobranego archiwum). Projekt nadrzędny może dostarczyć własne cele
# raylib / nlohmann_json::nlohmann_json / box2d::box2d - wtedy nic nie jest pobierane.
include(FetchContent)

# raylib 6.0
if(NOT TARGET raylib)
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
        URL_HASH SHA256=2b3ee1e2120c7a0796b33062c7e9a694dd8a8caa56a96319ac8c8ecf54a90d0b
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

# nlohmann/json 3.12.0 - wczytywanie arkuszy sprite'ów z JSON (Aseprite, TexturePacker). Archiwum
# json.tar.xz z wydania zawiera tylko nagłówki i CMake (~100 KB zamiast całego repozytorium).
if(NOT TARGET nlohmann_json::nlohmann_json)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
        URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Box2D 3.1.1 - fizyka brył sztywnych (biblioteka w C). Testy, przykłady i benchmarki nie są budowane.
if(NOT TARGET box2d::box2d)
    FetchContent_Declare(
        box2d
        URL https://github.com/erincatto/box2d/archive/refs/tags/v3.1.1.tar.gz
        URL_HASH SHA256=fb6ef914b50f4312d7d921a600eabc12318bb3c55a0b8c0b90608fa4488ef2e4
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
    set(BOX2D_BENCHMARKS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(box2d)
endif()

# Wspólne ostrzeżenia kompilatora dla biblioteki i demo (cel bez źródeł).
add_library(raycade_warnings INTERFACE)
if(MSVC)
    target_compile_options(raycade_warnings INTERFACE /W4)
else()
    target_compile_options(raycade_warnings INTERFACE -Wall -Wextra -Wpedantic -Wshadow)
endif()

add_subdirectory(engine)
if(RAYCADE_BUILD_DEMO)
    add_subdirectory(demo)
endif()
