/**
 * @file AudioScene.cpp
 * @brief Pokaz dźwięku: Engine::Audio(), Voice, szyny miksera (AudioBus), muzyka
 * i dźwięk w przestrzeni (audio::Emitter, audio::Listener).
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
 * - na dole jest świat: postać (strzałki w lewo/prawo) ma słuchacza (audio::Listener), a jej
 *   kroki grają ze zdarzeń klatek animacji (OnFrameEvent()); ognisko to emiter z zapętlonym
 *   dźwiękiem - słychać je głośniej i z właściwej strony, gdy postać podejdzie,
 * - C zrzuca skrzynię (RigidBody) - uderzenie o podłogę gra z głośnością z prędkości,
 * - X wywołuje wybuch: jednorazowy emiter usuwający się w OnFinish(),
 * - R wczytuje scenę od nowa: głosy sceny (ognisko) cichną, a muzyka gra dalej,
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

/** @brief Poziom podłogi świata. */
constexpr float kFloor = 600.0f;

/** @brief Skrzynia, która gra uderzenie z głośnością zależną od prędkości. */
class Crate : public RigidBody {
  public:
    /** @brief Buduje collider i grafikę oraz reakcję na zderzenie. */
    Crate() {
        Mass(1).Friction(0.6f).Bounce(0.2f);
        Spawn<collider::Rect>().Position({-14, -14}).Size({28, 28}).Paint(BROWN).Filled().Visible();
        Spawn<shapes::Rect>().Position({-14, -14}).Size({28, 28}).Paint(DARKBROWN);
        OnCollisionEnter([](Crate& self, const Collision&) {
            if (self.speed_ < 60.0f) return; // toczenie i drobne stuknięcia bez dźwięku
            Engine::Get()
                .Audio("uderzenie")
                .Play() // ustawienia tylko tego odtworzenia: mocniej - głośniej i niżej
                .Volume(std::clamp(self.speed_ / 700.0f, 0.15f, 1.0f))
                .Pitch(1.25f - std::clamp(self.speed_ / 1600.0f, 0.0f, 0.4f));
        });
    }

  protected:
    /** @brief Zapamiętuje prędkość przed krokiem fizyki - w zdarzeniu jest już po odbiciu. */
    void FixedTick(float, const Input&) override {
        const Vector2 v = Velocity();
        speed_ = std::sqrt(v.x * v.x + v.y * v.y);
    }

  private:
    float speed_ = 0.0f; /**< Prędkość z poprzedniego kroku. */
};

/** @brief Wybuch w danym miejscu: jednorazowy emiter z błyskiem, który znika po dźwięku. */
void Explosion(Scene& scene, Vector2 where) {
    auto& boom = scene.Spawn<audio::Emitter>()
                     .Audio("uderzenie")
                     .MaxDistance(700)
                     .Position(where)
                     .Play()
                     .OnFinish([](audio::Emitter& self) { self.Destroy(); });
    boom.Spawn<shapes::Circle>().Radius(18).Paint(Fade(ORANGE, 0.7f)).Filled();
}

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
                 "M zmiana muzyki (przenikanie), Enter pauza ścieżki, S stop muzyki, B wyciszenie muzyki\n"
                 "Strzałki lewo/prawo postać (słuchacz), C skrzynia, X wybuch, R scena od nowa")
        .FontSize(16)
        .Paint(GRAY)
        .LineHeight(1.4f)
        .Position({40, 110});

    SoundButton(scene, {40, 210}, "Moneta", "coin.wav");
    SoundButton(scene, {240, 210}, "Kroki", "kroki");
    SoundButton(scene, {440, 210}, "Uderzenie", "uderzenie");

    auto& status = scene.Spawn<visual::Text>().FontSize(18).Position({40, 70});
    auto& coinInfo = scene.Spawn<visual::Text>().FontSize(16).Paint(GRAY).Position({40, 285});

    // muzyka: szyna Exclusive() - ta sama ścieżka nie zaczyna się od nowa przy powrocie na scenę
    engine.Audio("theme.wav").Play();

    // ---------------- świat: dźwięk w przestrzeni ----------------
    scene.Gravity({0, 1400});
    scene.Spawn<StaticBody>().Position({0, kFloor}).Children([](StaticBody& floor) {
        floor.Spawn<collider::Rect>().Size({1000, 20}).Paint(LIGHTGRAY).Filled().Visible();
    });

    // ognisko: emiter z zapętlonym dźwiękiem jako dziecko grafiki - gra tam, gdzie ona jest
    auto& fire = scene.Spawn<shapes::Circle>()
                     .Position({840, kFloor - 14})
                     .Radius(14)
                     .Paint(ORANGE)
                     .Filled()
                     .OnUpdate([time = 0.0f](shapes::Circle& self, float dt) mutable {
                         time += dt;
                         self.Radius(13 + 2 * std::sin(time * 17) + std::sin(time * 5));
                     });
    fire.Spawn<audio::Emitter>()
        .Audio("ogien")
        .MaxDistance(650)
        .Attenuation(audio::Falloff::Smooth)
        .Play();
    scene.Spawn<visual::Text>().Content("ognisko").FontSize(14).Paint(GRAY).Position({812, 626});

    // postać: kroki ze zdarzeń klatek, słuchacz jako dziecko - uszy idą z postacią
    auto sheet = std::make_shared<SpriteSheet>(*engine.Assets().Sheet("sprite-test.json"));
    sheet->Event("biegnie", 1, "krok").Event("biegnie", 5, "krok");
    auto& hero = scene.Spawn<visual::Sprite>()
                     .Sheet(sheet)
                     .Origin({16, 32})
                     .Scale({2.5f, 2.5f})
                     .Position({160, kFloor})
                     .Play("stoi")
                     .OnFrameEvent([](visual::Sprite&, const std::string& event) {
                         if (event == "krok") Engine::Get().Audio("kroki").Play();
                     });
    hero.Spawn<audio::Listener>();

    // stan sceny trzymany w lambdzie: uchwyt ostatniej monety i licznik skończonych
    auto finished = std::make_shared<int>(0);
    scene.OnUpdate([&engine, &scene, &status, &coinInfo, &hero, finished,
                    lastCoin = Voice{}](float dt) mutable {
        const Input& input = engine.Input();

        // postać: ruch i animacja (kroki grają same, ze zdarzeń klatek)
        const float move = (input.KeyDown(KEY_RIGHT) ? 1.0f : 0.0f) -
                           (input.KeyDown(KEY_LEFT) ? 1.0f : 0.0f);
        const float x = std::clamp(hero.Position().x + move * 220.0f * dt, 30.0f, 970.0f);
        hero.Position({x, kFloor});
        if (move != 0.0f) hero.FlippedX(move < 0.0f);
        const char* animation = move != 0.0f ? "biegnie" : "stoi";
        if (hero.CurrentAnimation() != animation) hero.Play(animation);

        if (input.KeyPressed(KEY_C)) scene.Spawn<Crate>().Position({x, 330});
        if (input.KeyPressed(KEY_X))
            Explosion(scene, {static_cast<float>(engine.RandomInt(40, 960)), kFloor - 20});
        if (input.KeyPressed(KEY_R)) engine.ReloadScene("dzwiek");

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
