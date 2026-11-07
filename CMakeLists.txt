cmake_minimum_required(VERSION 3.24)
project(raycade LANGUAGES C CXX)

# Wymagany standard (C++23, deducing this) ustawia sama biblioteka przez
# target_compile_features - wystarczy GCC 14+, Clang 18+ albo MSVC z VS 2022 17.2+.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Demo budujemy tylko wtedy, gdy ten katalog jest głównym projektem. Inny projekt
# (np. silnik 3D) może dołączyć bibliotekę przez add_subdirectory() bez scen demo.
option(RAYCADE_BUILD_DEMO "Buduj program demonstracyjny" ${PROJECT_IS_TOP_LEVEL})

# raylib z systemu (devenv/Nix). Jeśli go nie ma, np. u prowadzącego
# na Windowsie, pobieramy i kompilujemy go automatycznie. Projekt nadrzędny
# może też dostarczyć własny cel raylib.
if(NOT TARGET raylib)
    find_package(raylib 5.0 QUIET)
endif()
if(NOT TARGET raylib)
    message(STATUS "raylib nie znaleziony w systemie - pobieranie przez FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
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
