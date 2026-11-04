cmake_minimum_required(VERSION 3.24)
project(silnik LANGUAGES C CXX)

# C++23: jawny parametr obiektu (deducing this) w setterach klas bazowych.
# Wymaga GCC 14+, Clang 18+ albo MSVC z Visual Studio 2022 17.2+.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# raylib z systemu (devenv/Nix). Jeśli go nie ma, np. u prowadzącego
# na Windowsie, pobieramy i kompilujemy go automatycznie.
find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
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

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(silnik ${SOURCES})
target_include_directories(silnik PRIVATE src)
target_link_libraries(silnik PRIVATE raylib)

if(MSVC)
    target_compile_options(silnik PRIVATE /W4 /utf-8)
else()
    target_compile_options(silnik PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Zasoby (bitmapy, sprite sheety) kopiowane obok pliku wykonywalnego
if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    add_custom_command(TARGET silnik POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:silnik>/assets)
endif()
