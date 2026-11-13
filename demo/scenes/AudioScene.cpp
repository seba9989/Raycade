/**
 * @file AudioScene.cpp
 * @brief Pokaz dźwięku: Engine::Audio() (audio::Sound, audio::Music, audio::Voice), szyny
 * miksera (audio::Bus) i dźwięk w przestrzeni (node::sound::Emitter, node::sound::Listener).
 * @details Dźwięki, muzyka i szyny są skonfigurowane raz, w main.cpp - scena tylko je odtwarza.
 * - przyciski albo klawisze odtwarzają dźwięki: spacja moneta (losowa wysokość), K kroki
 *   (losowy plik z trzech), U uderzenie,
 * - Q odtwarza monetę z wysokością zmienioną tylko dla tego odtworzenia (Voice::Pitch()),
 * - P wstrzymuje szynę "efekty" razem z jej dzieckiem "kroki" (szyna "interfejs" gra dalej),
 * - N wycisza szynę "efekty", strzałki w górę i w dół zmieniają głośność szyny głównej,
 * - muzyka startuje razem ze sceną; M przełącza ścieżkę theme/boss - szyna "music" ma
 *   Exclusive(), więc stara ścieżka się wycisza (FadeOutTime()), a nowa narasta (FadeInTime()),
 *   S zatrzymuje muzykę z wyciszeniem, B wycisza samą szynę muzyki,
 * - Enter wstrzymuje i wznawia bieżącą ścieżkę (Music::Pause() / Music::Resume()),
 * - moneta ma MaxVoices(4) i MinInterval(): przytrzymana spacja nie zagłusza reszty; Backspace
 *   ucina ostatnią monetę przez zapamiętany uchwyt Voice,
 * - na dole jest świat: postać (strzałki w lewo/prawo) ma słuchacza (node::sound::Listener), a jej
 *   kroki grają ze zdarzeń klatek animacji (OnFrameEvent()); ognisko to emiter z zapętlonym
 *   dźwiękiem - słychać je głośniej i z właściwej strony, gdy postać podejdzie,
 * - C zrzuca skrzynię (RigidBody) - uderzenie o podłogę gra z głośnością z prędkości,
 * - X wywołuje wybuch: jednorazowy emiter usuwający się w OnFinish(),
 * - R wczytuje scenę od nowa: głosy sceny (ognisko) cichną, a muzyka (Global()) gra dalej,
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
class Crate : public node::physics::RigidBody {
  public:
    /** @brief Buduje collider i grafikę oraz reakcję na zderzenie. */
    Crate() {
        Mass(1).Friction(0.6f).Bounce(0.2f);
        Spawn<node::collider::Rect>()
            .Position({-14, -14})
            .Size({28, 28})
            .Paint(BROWN)
            .Filled()
            .Visible();
        Spawn<node::shapes::Rect>().Position({-14, -14}).Size({28, 28}).Paint(DARKBROWN);
        OnCollisionEnter([](Crate& self, const node::physics::Collision&) {
            if (self.speed_ < 60.0f) return; // toczenie i drobne stuknięcia bez dźwięku
            Engine::Get()
                .Audio()
                .Sound("uderzenie")
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
    auto& boom = scene.Spawn<node::sound::Emitter>()
                     .Sound("uderzenie")
                     .MaxDistance(700)
                     .Position(where)
                     .Play()
                     .OnFinish([](node::sound::Emitter& self) { self.Destroy(); });
    boom.Spawn<node::shapes::Circle>().Radius(18).Paint(Fade(ORANGE, 0.7f)).Filled();
}

/** @brief Dodaje przycisk, który odtwarza dźwięk (i klik interfejsu). */
void SoundButton(Scene& scene, Vector2 position, const std::string& label, std::string sound) {
    auto& button = scene.Spawn<node::shapes::Rect>()
                       .Position(position)
                       .Size({180, 56})
                       .Paint(DARKBLUE)
                       .OnMouseEnter([](node::shapes::Rect& self) { self.Filled(); })
                       .OnMouseLeave([](node::shapes::Rect& self) { self.Filled(false); })
                       .OnClick([sound = std::move(sound)](node::shapes::Rect&) {
                           Engine::Get().Audio().Sound("klik").Play();
                           Engine::Get().Audio().Sound(sound).Play();
                       });
    button.Spawn<node::visual::Text>()
        .Content(label)
        .FontSize(20)
        .Paint(GRAY)
        .Anchor({0.5f, 0.5f})
        .Position({90, 28});
}

/** @brief Opis stanu szyny do napisu: wyciszona, wstrzymana albo głośność w procentach. */
std::string BusLabel(const audio::Bus& bus) {
    if (bus.IsMuted()) return "wyciszona";
    if (bus.IsPaused()) return "wstrzymana";
    return std::format("{}%", std::lround(bus.Volume() * 100));
}

} // namespace

/** @brief Konfiguruje scenę dźwięku. */
SCENE("dzwiek") {
    scene.Spawn<node::visual::Text>().Content("Dźwięk").FontSize(28).Position({40, 30});
    scene.Spawn<node::visual::Text>()
        .Content(
            "Spacja moneta, K kroki, U uderzenie, Q moneta o kwintę wyżej, Backspace ucina monetę\n"
            "P pauza efektów, N wyciszenie efektów, strzałki góra/dół głośność główna\n"
            "M zmiana muzyki (przenikanie), Enter pauza ścieżki, S stop muzyki, B wyciszenie "
            "muzyki\n"
            "Strzałki lewo/prawo postać (słuchacz), C skrzynia, X wybuch, R scena od nowa"
        )
        .FontSize(16)
        .Paint(GRAY)
        .LineHeight(1.4f)
        .Position({40, 110});

    SoundButton(scene, {40, 210}, "Moneta", "coin.wav");
    SoundButton(scene, {240, 210}, "Kroki", "kroki");
    SoundButton(scene, {440, 210}, "Uderzenie", "uderzenie");

    auto& status = scene.Spawn<node::visual::Text>().FontSize(18).Position({40, 70});
    auto& coinInfo = scene.Spawn<node::visual::Text>().FontSize(16).Paint(GRAY).Position({40, 285});

    // muzyka jest globalna: po powrocie na scenę albo R gra dalej ta ścieżka, która grała
    if (engine.Audio().Bus(Audio::kMusicBus).VoiceCount() == 0)
        engine.Audio().Music("theme.wav").Play();

    // ---------------- świat: dźwięk w przestrzeni ----------------
    scene.Gravity({0, 1400});
    scene.Spawn<node::physics::StaticBody>()
        .Position({0, kFloor})
        .Children([](node::physics::StaticBody& floor) {
            floor.Spawn<node::collider::Rect>()
                .Size({1000, 20})
                .Paint(LIGHTGRAY)
                .Filled()
                .Visible();
        });

    // ognisko: emiter z zapętlonym dźwiękiem jako dziecko grafiki - gra tam, gdzie ona jest
    auto& fire = scene.Spawn<node::shapes::Circle>()
                     .Position({840, kFloor - 14})
                     .Radius(14)
                     .Paint(ORANGE)
                     .Filled()
                     .OnUpdate([time = 0.0f](node::shapes::Circle& self, float dt) mutable {
                         time += dt;
                         self.Radius(13 + 2 * std::sin(time * 17) + std::sin(time * 5));
                     });
    fire.Spawn<node::sound::Emitter>()
        .Sound("ogien")
        .MaxDistance(650)
        .Attenuation(node::sound::Falloff::Smooth)
        .Play();
    scene.Spawn<node::visual::Text>().Content("ognisko").FontSize(14).Paint(GRAY).Position(
        {812, 626}
    );

    // postać: kroki ze zdarzeń klatek, słuchacz jako dziecko - uszy idą z postacią
    auto sheet = std::make_shared<SpriteSheet>(*engine.Assets().Sheet("sprite-test.json"));
    sheet->Event("biegnie", 1, "krok").Event("biegnie", 5, "krok");
    auto& hero = scene.Spawn<node::visual::Sprite>()
                     .Sheet(sheet)
                     .Origin({16, 32})
                     .Scale({2.5f, 2.5f})
                     .Position({160, kFloor})
                     .Play("stoi")
                     .OnFrameEvent([&engine](node::visual::Sprite&, const std::string& event) {
                         if (event == "krok") engine.Audio().Sound("kroki").Play();
                     });
    hero.Spawn<node::sound::Listener>();

    // stan sceny trzymany w lambdzie: uchwyt ostatniej monety i licznik skończonych
    auto finished = std::make_shared<int>(0);
    scene.OnUpdate([&engine, &scene, &status, &coinInfo, &hero, finished,
                    lastCoin = audio::Voice{}](float dt) mutable {
        const Input& input = engine.Input();
        Audio& audio = engine.Audio();

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
            lastCoin = audio.Sound("coin.wav").Play().OnFinish([finished](audio::Voice&) {
                ++*finished;
            });
        if (input.KeyPressed(KEY_BACKSPACE)) lastCoin.Stop(); // martwy uchwyt - bez efektu
        if (input.KeyPressed(KEY_K)) audio.Sound("kroki").Play();
        if (input.KeyPressed(KEY_U)) audio.Sound("uderzenie").Play();
        // zmiana tylko tego jednego odtworzenia - ustawienia dźwięku zostają bez zmian
        if (input.KeyPressed(KEY_Q)) audio.Sound("coin.wav").Play().Pitch(1.5f);

        audio::Music& theme = audio.Music("theme.wav");
        audio::Music& boss = audio.Music("boss.wav");
        // bieżąca ścieżka: boss, jeśli ma odtworzenie (także wstrzymane), inaczej theme
        audio::Music& track = boss.VoiceCount() > 0 ? boss : theme;
        if (input.KeyPressed(KEY_M)) (&track == &boss ? theme : boss).Play(); // stara się wycisza
        if (input.KeyPressed(KEY_ENTER)) {
            if (track.IsPaused())
                track.Resume();
            else
                track.Pause();
        }
        audio::Bus& music = audio.Bus(Audio::kMusicBus);
        if (input.KeyPressed(KEY_S)) music.Stop();
        if (input.KeyPressed(KEY_B)) music.Muted(!music.IsMuted());

        audio::Bus& effects = audio.Bus("efekty");
        if (input.KeyPressed(KEY_P)) effects.Paused(!effects.IsPaused());
        if (input.KeyPressed(KEY_N)) effects.Muted(!effects.IsMuted());
        if (input.KeyPressed(KEY_UP)) audio.Volume(std::min(audio.Volume() + 0.1f, 1.0f));
        if (input.KeyPressed(KEY_DOWN)) audio.Volume(std::max(audio.Volume() - 0.1f, 0.0f));

        status.Content(
            "Główna: {}    efekty: {}    kroki: {}    muzyka: {}    głosy: {}",
            BusLabel(audio.Bus()), BusLabel(effects), BusLabel(audio.Bus("kroki")), BusLabel(music),
            audio.VoiceCount()
        );
        coinInfo.Content(
            "Ostatnia moneta: {:.2f} / {:.2f} s    skończone monety: {}", lastCoin.PlaybackTime(),
            lastCoin.Duration(), *finished
        );
    });
}
