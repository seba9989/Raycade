/**
 * @file SceneMacro.hpp
 * @brief Rejestracja scen: SceneRegistration i makro SCENE.
 * @details Wymaga pełnej definicji klasy Engine, więc sam dołącza Engine.hpp (nigdy
 * odwrotnie). Do gry trafia przez nagłówek zbiorczy `<raycade>`.
 */
#pragma once

#include <raycade/engine/Engine.hpp>

/**
 * @brief Pomocniczy rejestrator scen wykorzystywany przez makro SCENE.
 * @details Statyczny obiekt tej klasy powstaje przed main(), a jego konstruktor rejestruje
 * funkcję konfigurującą scenę w silniku.
 */
struct SceneRegistration {
    /**
     * @brief Zapamiętuje funkcję konfigurującą scenę.
     * @param name Nazwa sceny.
     * @param setup Funkcja konfigurująca; dostaje scenę i silnik.
     */
    SceneRegistration(const char* name, void (*setup)(Scene&, Engine&)) {
        Engine::Get().RegisterScene(name, [setup](Scene& scene) { setup(scene, Engine::Get()); });
    }
};

/** @cond PRYWATNE */
#define SCENE_CONCAT_(a, b) a##b
#define SCENE_CONCAT(a, b) SCENE_CONCAT_(a, b)
#define SCENE_IMPL_(name, setup)                                                                   \
    static void setup(Scene& scene, [[maybe_unused]] Engine& engine);                              \
    static const SceneRegistration SCENE_CONCAT(setup, _registration){name, setup};                \
    static void setup(Scene& scene, [[maybe_unused]] Engine& engine)
/** @endcond */

/**
 * @def SCENE(name)
 * @brief Rejestruje scenę z bloku kodu wykonywanego po inicjalizacji okna.
 * @details Wewnątrz bloku dostępne są zmienne `scene` (Scene&) i `engine` (Engine&, to samo co
 * Engine::Get()). Lambdy z bloku mogą przechwycić `[&engine]` - silnik żyje do końca programu,
 * więc ta referencja nigdy się nie unieważni. Blok wykona się przy pierwszym wejściu na scenę
 * (wczytywanie leniwe), a scenę startową wybiera Engine::StartScene(). Makro jest przeznaczone dla
 * plików .cpp gry - nie należy go używać w bibliotece statycznej, bo linker może pominąć plik, do
 * którego nic się nie odwołuje.
 * @code{.cpp}
 * SCENE("menu") {
 *     scene.ProvideContext<DefaultTypeface>(engine.Assets().Font("font.ttf"));
 *     scene.Spawn<node::shapes::Rect>().Position({300, 200}).Size({400, 300})
 *         .OnClick([&engine](node::shapes::Rect&) { engine.SwitchScene("gra"); });
 * }
 * @endcode
 * @param name Nazwa sceny używana przez StartScene() i SwitchScene().
 */
#define SCENE(name) SCENE_IMPL_(name, SCENE_CONCAT(scene_setup_, __COUNTER__))
