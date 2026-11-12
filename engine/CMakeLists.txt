# Biblioteka Raycade (bez scen - te należą do gry, patrz makro SCENE). Źródła zbiera glob jak
# w demo; CONFIGURE_DEPENDS sprawia, że nowy albo usunięty plik .cpp w src/ jest wykrywany przy
# kolejnej kompilacji, bez ręcznego ponownego uruchamiania CMake.
file(GLOB_RECURSE RAYCADE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(raycade STATIC ${RAYCADE_SOURCES})
add_library(raycade::raycade ALIAS raycade)

# PUBLIC: gra dołącza <raycade> albo <raycade/...> i dostaje te same ustawienia co biblioteka.
# Kolejność ma znaczenie: umbrella/ (plik "raycade") przed include/ (katalog "raycade/"),
# żeby kompilator znalazł plik, zanim natrafi na katalog o tej samej nazwie.
target_include_directories(raycade PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/umbrella
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# C++23 (deducing this) jest używany w nagłówkach, więc wymóg przechodzi na grę.
target_compile_features(raycade PUBLIC cxx_std_23)

# raylib jest częścią API (Color, Vector2, KEY_*), dlatego PUBLIC.
# nlohmann/json (SpriteSheet.cpp) i Box2D (src/physics) tylko w implementacji - nie trafiają
# do nagłówków API, więc gra ich nie widzi.
target_link_libraries(
    raycade PUBLIC raylib PRIVATE raycade_warnings nlohmann_json::nlohmann_json box2d::box2d
)

# Nagłówki zawierają polskie znaki (UTF-8) - MSVC bez /utf-8 czyta je w stronie kodowej
# systemu, więc opcja musi trafić także do każdego pliku gry, który je dołącza.
if(MSVC)
    target_compile_options(raycade PUBLIC /utf-8)
endif()
