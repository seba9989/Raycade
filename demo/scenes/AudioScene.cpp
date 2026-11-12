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
 * - Enter wstrzymuje i wznawia bieżącą ścieżkę (Voice::Pause() / Voice::Play()) - Play() na
 *   szynie Exclusive() zwraca głos, który już gra,
 * - moneta ma MaxVoices(4) i MinInterval(): przytrzymana spacja nie zagłusza reszty; Backspace
 *   ucina ostatnią monetę przez zapamiętany uchwyt Voice,
 * - napis u góry pokazuje stan szyn, liczbę trwających głosów, czas ostatniej monety
 *   (Voice::PlaybackTime() / Duration()) i liczbę skończonych monet (Voice::OnFinish()).
 */

#include <raycade>

#include <algorithm>
#include <cmath>
#include <format>
#include <memory>
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
        .Content("Spacja moneta, K kroki, U uderzenie, Q moneta o kwintę wyżej, Backspace ucina monetę\n"
                 "P pauza efektów, N wyciszenie efektów, strzałki góra/dół głośność główna\n"
                 "M zmiana muzyki (przenikanie), Enter pauza ścieżki, S stop muzyki, B wyciszenie muzyki")
        .FontSize(16)
        .Paint(GRAY)
        .LineHeight(1.4f)
        .Position({40, 110});

    SoundButton(scene, {40, 180}, "Moneta", "coin.wav");
    SoundButton(scene, {240, 180}, "Kroki", "kroki");
    SoundButton(scene, {440, 180}, "Uderzenie", "uderzenie");

    auto& status = scene.Spawn<visual::Text>().FontSize(18).Position({40, 70});
    auto& coinInfo = scene.Spawn<visual::Text>().FontSize(16).Paint(GRAY).Position({40, 260});

    // muzyka: szyna Exclusive() - ta sama ścieżka nie zaczyna się od nowa przy powrocie na scenę
    engine.Audio("theme.wav").Play();

    // stan sceny trzymany w lambdzie: uchwyt ostatniej monety i licznik skończonych
    auto finished = std::make_shared<int>(0);
    scene.OnUpdate([&engine, &status, &coinInfo, finished, lastCoin = Voice{}](float) mutable {
        const Input& input = engine.Input();
        if (input.KeyDown(KEY_SPACE)) // przytrzymana: limit głosów i MinInterval() pilnują hałasu
            lastCoin = engine.Audio("coin.wav").Play().OnFinish([finished](Voice&) { ++*finished; });
        if (input.KeyPressed(KEY_BACKSPACE)) lastCoin.Stop(); // martwy uchwyt - bez efektu
        if (input.KeyPressed(KEY_K)) engine.Audio("kroki").Play();
        if (input.KeyPressed(KEY_U)) engine.Audio("uderzenie").Play();
        // zmiana tylko tego jednego odtworzenia - ustawienia dźwięku zostają bez zmian
        if (input.KeyPressed(KEY_Q)) engine.Audio("coin.wav").Play().Pitch(1.5f);

        if (input.KeyPressed(KEY_M)) {
            const bool boss = engine.Audio("boss.wav").IsPlaying();
            engine.Audio(boss ? "theme.wav" : "boss.wav").Play(); // stara ścieżka się wycisza
        }
        if (input.KeyPressed(KEY_ENTER)) {
            // Play() na szynie Exclusive() zwraca głos, który już gra - to jest bieżąca ścieżka
            const bool boss = engine.Audio("boss.wav").VoiceCount() > 0; // także wstrzymany
            Voice track = engine.Audio(boss ? "boss.wav" : "theme.wav").Play();
            if (track.IsPaused())
                track.Play();
            else
                track.Pause();
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
        coinInfo.Content(
            "Ostatnia moneta: " + std::format("{:.2f} / {:.2f} s", lastCoin.PlaybackTime(),
                                               lastCoin.Duration()) +
            "    skończone monety: " + std::to_string(*finished)
        );
    });
}
