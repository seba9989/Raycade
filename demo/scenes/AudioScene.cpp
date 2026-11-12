/**
 * @file AudioScene.cpp
 * @brief Pokaz dźwięku: Engine::Audio(), Voice, szyny miksera (AudioBus) i muzyka.
 * @details Dźwięki i szyny są skonfigurowane raz, w main.cpp - scena tylko je odtwarza.
 * - przyciski albo klawisze odtwarzają dźwięki: spacja moneta (losowa wysokość), K kroki
 *   (losowy plik z trzech), U uderzenie,
 * - Q odtwarza monetę z wysokością zmienioną tylko dla tego odtworzenia (Voice::Pitch()),
 * - P wstrzymuje szynę "efekty" razem z jej dzieckiem "kroki" (szyna "interfejs" gra dalej),
 * - N wycisza szynę "efekty", strzałki w górę i w dół zmieniają głośność szyny głównej,
 * - muzyka startuje razem ze sceną; M przełącza ścieżkę theme/boss - szyna "muzyka" ma
 *   Exclusive(), więc stara ścieżka się wycisza (FadeOutTime()), a nowa narasta (FadeInTime()),
 *   S zatrzymuje muzykę z wyciszeniem, B wycisza samą szynę muzyki,
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
                 "P pauza efektów, N wyciszenie efektów, strzałki góra/dół głośność główna\n"
                 "M zmiana muzyki (przenikanie), S stop muzyki, B wyciszenie muzyki")
        .FontSize(16)
        .Paint(GRAY)
        .LineHeight(1.4f)
        .Position({40, 110});

    SoundButton(scene, {40, 180}, "Moneta", "coin.wav");
    SoundButton(scene, {240, 180}, "Kroki", "kroki");
    SoundButton(scene, {440, 180}, "Uderzenie", "uderzenie");

    auto& status = scene.Spawn<visual::Text>().FontSize(18).Position({40, 70});

    // muzyka: szyna Exclusive() - ta sama ścieżka nie zaczyna się od nowa przy powrocie na scenę
    engine.Audio("theme.wav").Play();

    scene.OnUpdate([&engine, &status](float) {
        const Input& input = engine.Input();
        if (input.KeyPressed(KEY_SPACE)) engine.Audio("coin.wav").Play();
        if (input.KeyPressed(KEY_K)) engine.Audio("kroki").Play();
        if (input.KeyPressed(KEY_U)) engine.Audio("uderzenie").Play();
        // zmiana tylko tego jednego odtworzenia - ustawienia dźwięku zostają bez zmian
        if (input.KeyPressed(KEY_Q)) engine.Audio("coin.wav").Play().Pitch(1.5f);

        if (input.KeyPressed(KEY_M)) {
            const bool boss = engine.Audio("boss.wav").IsPlaying();
            engine.Audio(boss ? "theme.wav" : "boss.wav").Play(); // stara ścieżka się wycisza
        }
        if (input.KeyPressed(KEY_S)) engine.AudioBus("muzyka").Stop();
        AudioBus& music = engine.AudioBus("muzyka");
        if (input.KeyPressed(KEY_B)) music.Muted(!music.IsMuted());

        AudioBus& master = engine.AudioBus();
        AudioBus& effects = engine.AudioBus("efekty");
        if (input.KeyPressed(KEY_P)) effects.Paused(!effects.IsPaused());
        if (input.KeyPressed(KEY_N)) effects.Muted(!effects.IsMuted());
        if (input.KeyPressed(KEY_UP)) master.Volume(std::min(master.Volume() + 0.1f, 1.0f));
        if (input.KeyPressed(KEY_DOWN)) master.Volume(std::max(master.Volume() - 0.1f, 0.0f));

        status.Content(
            "Główna: " + BusLabel(master) + "    efekty: " + BusLabel(effects) +
            "    kroki: " + BusLabel(engine.AudioBus("kroki")) + "    muzyka: " + BusLabel(music) +
            "    głosy: " + std::to_string(master.VoiceCount())
        );
    });
}
