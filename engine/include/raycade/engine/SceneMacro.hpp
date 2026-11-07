/**
 * @file SceneMacro.hpp
 * @brief Rejestracja scen: SceneRegistration i makro SCENE.
 * @details Wymaga pełnej definicji klasy Engine, więc sam dołącza Engine.hpp (nigdy
 * odwrotnie). Do gry trafia przez nagłówek zbiorczy `<raycade>`.
 */
#pragma once

#include <raycade/Engine.hpp>

/**
 * @brief Pomocniczy rejestrator scen wykorzystywany przez makro SCENE.
 * @details Statyczny obiekt tej klasy powstaje przed main(), a jego konstruktor rejestruje
 * funkcję konfigurującą scenę w silniku.
 */
struct SceneRegistration {
    /**
     * @brief Zapamiętuje funkcję konfigurującą scenę.
     * @param name Nazwa sceny.
     * @param setup Funkcja konfigurująca.
     */
    SceneRegistration(const char* name, void (*setup)(Scene&)) {
        Engine::Get().RegisterScene(name, setup);
    }
};

/** @cond PRYWATNE */
#define SCENE_CONCAT_(a, b) a##b
#define SCENE_CONCAT(a, b) SCENE_CONCAT_(a, b)
#define SCENE_IMPL_(name, setup)                                                                   \
    static void setup(Scene& scene);                                                               \
    static const SceneRegistration SCENE_CONCAT(setup, _registration){name, setup};                \
    static void setup(Scene& scene)
/** @endcond */

/**
 * @def SCENE(name)
 * @brief Rejestruje scenę z bloku kodu wykonywanego po inicjalizacji okna.
 * @details Wewnątrz bloku dostępna jest zmienna `scene` (Scene&). Blok wykona się przy
 * pierwszym wejściu na scenę (wczytywanie leniwe), a scenę startową wybiera
 * Engine::StartScene(). Makro jest przeznaczone dla plików .cpp gry - nie należy go używać
 * w bibliotece statycznej, bo linker może pominąć plik, do którego nic się nie odwołuje.
 * @code{.cpp}
 * SCENE("menu") {
 *     scene.Spawn<shapes::Rect>().Position({300, 200}).Size({400, 300});
 * }
 * @endcode
 * @param name Nazwa sceny używana przez StartScene() i SwitchScene().
 */
#define SCENE(name) SCENE_IMPL_(name, SCENE_CONCAT(scene_setup_, __COUNTER__))
