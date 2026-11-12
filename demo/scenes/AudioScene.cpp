/**
 * @file AudioScene.cpp
 * @brief Pokaz dźwięku: Engine::Audio(), Voice i szyny miksera (AudioBus).
 * @details Dźwięki i szyny są skonfigurowane raz, w main.cpp - scena tylko je odtwarza.
 * - przyciski albo klawisze odtwarzają dźwięki: spacja moneta (losowa wysokość), K kroki
 *   (losowy plik z trzech), U uderzenie,
 * - Q odtwarza monetę z wysokością zmienioną tylko dla tego odtworzenia (Voice::Pitch()),
 * - P wstrzymuje szynę "efekty" razem z jej dzieckiem "kroki" (szyna "interfejs" gra dalej),
 * - N wycisza szynę "efekty", strzałki w górę i w dół zmieniają głośność szyny głównej,
 * - napis u góry pokazuje stan szyn i liczbę trwających głosów.
 */

#include <raycade>

#include <algorithm>
#include <cmath>
#include <string>

namespace {

/** @brief Dodaje przycisk, który odtwarza dźwięk (i klik interfejsu). */
void SoundButton(Scene& scene, Vector2 position, const std::string& label, std::string sound) {
    auto& button = scene.Spawn<shapes::Rect>()
                       .Position(position)
                       .Size({180, 56})
                       .Paint(DARKBLUE)
                       .OnMouseEnter([](shapes::Rect& self) { self.Filled(); })
                       .OnMouseLeave([](shapes::Rect& self) { self.Filled(false); })
                       .OnClick([sound = std::move(sound)](shapes::Rect&) {
                           Engine::Get().Audio("klik").Play();
                           Engine::Get().Audio(sound).Play();
                       });
    button.Spawn<visual::Text>()
        .Content(label)
        .FontSize(20)
        .Paint(GRAY)
        .Anchor({0.5f, 0.5f})
        .Position({90, 28});
}

/** @brief Opis stanu szyny do napisu: wyciszona, wstrzymana albo głośność w procentach. */
std::string BusLabel(const AudioBus& bus) {
    if (bus.IsMuted()) return "wyciszona";
    if (bus.IsPaused()) return "wstrzymana";
    return std::to_string(static_cast<int>(std::lround(bus.Volume() * 100))) + "%";
}

} // namespace

/** @brief Konfiguruje scenę dźwięku. */
SCENE("dzwiek") {
    Engine& engine = Engine::Get();
    scene.ProvideContext<DefaultTypeface>(engine.Assets().Font("font.ttf"));

    scene.Spawn<visual::Text>().Content("Dźwięk").FontSize(28).Position({40, 30});
    scene.Spawn<visual::Text>()
        .Content("Spacja moneta, K kroki, U uderzenie, Q moneta o kwintę wyżej\n"
                 "P pauza efektów, N wyciszenie efektów, strzałki góra/dół głośność główna")
        .FontSize(16)
        .Paint(GRAY)
        .LineHeight(1.4f)
        .Position({40, 110});

    SoundButton(scene, {40, 180}, "Moneta", "coin.wav");
    SoundButton(scene, {240, 180}, "Kroki", "kroki");
    SoundButton(scene, {440, 180}, "Uderzenie", "uderzenie");

    auto& status = scene.Spawn<visual::Text>().FontSize(18).Position({40, 70});

    scene.OnUpdate([&engine, &status](float) {
        const Input& input = engine.Input();
        if (input.KeyPressed(KEY_SPACE)) engine.Audio("coin.wav").Play();
        if (input.KeyPressed(KEY_K)) engine.Audio("kroki").Play();
        if (input.KeyPressed(KEY_U)) engine.Audio("uderzenie").Play();
        // zmiana tylko tego jednego odtworzenia - ustawienia dźwięku zostają bez zmian
        if (input.KeyPressed(KEY_Q)) engine.Audio("coin.wav").Play().Pitch(1.5f);

        AudioBus& master = engine.AudioBus();
        AudioBus& effects = engine.AudioBus("efekty");
        if (input.KeyPressed(KEY_P)) effects.Paused(!effects.IsPaused());
        if (input.KeyPressed(KEY_N)) effects.Muted(!effects.IsMuted());
        if (input.KeyPressed(KEY_UP)) master.Volume(std::min(master.Volume() + 0.1f, 1.0f));
        if (input.KeyPressed(KEY_DOWN)) master.Volume(std::max(master.Volume() - 0.1f, 0.0f));

        status.Content(
            "Główna: " + BusLabel(master) + "    efekty: " + BusLabel(effects) +
            "    kroki: " + BusLabel(engine.AudioBus("kroki")) +
            "    głosy: " + std::to_string(master.VoiceCount())
        );
    });
}
