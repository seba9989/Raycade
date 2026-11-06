# Biblioteka Raycade (bez scen - te należą do gry, patrz makro SCENE).
add_library(raycade STATIC
    src/core/Engine.cpp
    src/render/Bitmap.cpp
    src/render/Geometry.cpp
    src/render/RenderTarget.cpp
    src/render/primitive_renderer/Fill.cpp
    src/render/primitive_renderer/Polygon.cpp
    src/render/primitive_renderer/Primitives.cpp
    src/render/primitive_renderer/RoundShapes.cpp
)
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
target_link_libraries(raycade PUBLIC raylib PRIVATE raycade_warnings)

# Nagłówki zawierają polskie znaki (UTF-8) - MSVC bez /utf-8 czyta je w stronie kodowej
# systemu, więc opcja musi trafić także do każdego pliku gry, który je dołącza.
if(MSVC)
    target_compile_options(raycade PUBLIC /utf-8)
endif()
