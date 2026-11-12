/**
 * @file EffectsScene.cpp
 * @brief Pokaz animacji bitmap, przycinania, shaderów, usuwania obiektów, kroku stałego i tekstu.
 * @details
 * - animacja: koło z arkusza 8 klatek (arkusz rysowany w programie na Bitmap); LPM na kole
 *   wstrzymuje i wznawia animację, X odbija drugie koło,
 * - przycinanie: lista w panelu z ClipRect() - kółko myszy przewija, wiersze poza panelem nie są
 *   widoczne ani klikalne,
 * - shadery: powierzchnia z falującym shaderem (uniform czasu), G włącza szarość całego obrazu
 *   (Engine::PostShader()),
 * - usuwanie i krok stały: piłki odbijają się z fizyką liczoną w OnFixedUpdate(); LPM na piłce
 *   usuwa ją (Destroy()), B albo podwójne kliknięcie w boisko (gest GESTURE_DOUBLETAP) dodaje
 *   nową,
 * - tekst: napisy to obiekty visual::Text rysowane czcionką z polskimi znakami z pliku
 *   `assets/font.ttf` (w repozytorium DejaVu Sans; bez niego - font raylib).
 */

#include <raycade>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace {

/** @brief Fala: przesunięcie współrzędnych tekstury zależne od czasu. */
constexpr const char* kWaveShader = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float time;
out vec4 finalColor;
void main() {
    vec2 uv = fragTexCoord;
    uv.x += sin(uv.y * 25.0 + time * 4.0) * 0.015;
    finalColor = texture(texture0, uv) * colDiffuse * fragColor;
})";

/** @brief Skala szarości całego obrazu. */
constexpr const char* kGrayShader = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
out vec4 finalColor;
void main() {
    vec4 color = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    finalColor = vec4(gray, gray, gray, color.a);
})";

/** @brief Boisko piłek (w pikselach gry). */
constexpr Rectangle kField{40, 380, 920, 250};

/** @brief Rozmiar klatki arkusza koła. */
constexpr float kFrame = 48.0f;

class Ball;

/** @brief Stan sceny w kontekście: lista piłek do fizyki w kroku stałym. */
struct Balls {
    std::vector<Ball*> list; /**< Piłki na boisku (bez usuniętych). */
    int removed = 0;         /**< Licznik usuniętych. */
};

/** @brief Piłka z prędkością; LPM ją usuwa. */
class Ball : public shapes::Circle {
  public:
    Ball() {
        Radius(18).Filled().OnClick([](Ball& self) {
            // najpierw z listy fizyki - po końcu klatki wskaźnik będzie nieważny
            if (Balls* balls = self.Context<Balls>()) {
                std::erase(balls->list, &self);
                ++balls->removed;
            }
            self.Destroy();
        });
    }

    Vector2 velocity{0, 0}; /**< Prędkość w pikselach na sekundę. */

  protected:
    void OnMount() override {
        if (Balls* balls = Context<Balls>()) balls->list.push_back(this);
    }
};

/** @brief Rysuje bitmapę 8 klatek koła z obracającą się szprychą (4 x 2 klatki). */
std::shared_ptr<const Bitmap> MakeWheelSheet() {
    Bitmap sheet{static_cast<int>(kFrame * 4), static_cast<int>(kFrame * 2), BLANK};
    PrimitiveRenderer renderer;
    renderer.Target(sheet);
    for (int i = 0; i < 8; ++i) {
        const Vector2 center{
            (static_cast<float>(i % 4) + 0.5f) * kFrame, (static_cast<float>(i / 4) + 0.5f) * kFrame
        };
        const Vector2 spoke{
            18 * std::cos(static_cast<float>(i) * PI / 4.0f),
            18 * std::sin(static_cast<float>(i) * PI / 4.0f)
        };
        renderer.Paint(ORANGE).Filled().Circle(center, 20);
        renderer.Paint(DARKBROWN).Filled(false).Circle(center, 20);
        renderer.Line(center, Vector2{center.x + spoke.x, center.y + spoke.y});
        renderer.Line(center, Vector2{center.x - spoke.x, center.y - spoke.y});
    }
    return std::make_shared<const Bitmap>(std::move(sheet));
}

/** @brief Dodaje piłkę w punkcie z losową prędkością i kolorem. */
void SpawnBall(Scene& scene, Vector2 position) {
    Engine& engine = Engine::Get();
    const Color colors[] = {RED, BLUE, DARKGREEN, PURPLE, ORANGE, MAROON};

    scene.Spawn<Ball>().Position(position).Paint(colors[engine.RandomValue(0, 5)]).velocity = {
        engine.RandomFloat(-250, 250), engine.RandomFloat(-300, -50)
    };
}

} // namespace

/** @brief Konfiguruje scenę efektów. */
SCENE("efekty") {
    Engine& engine = Engine::Get();
    scene.ProvideContext<Balls>();

    // czcionka z polskimi znakami (w repozytorium DejaVu Sans) - dla wszystkich napisów sceny.
    // Napisy mają polskie litery; bez pliku czcionki font raylib pokaże w ich miejscu "?",
    // a dolny napis (bez polskich liter) wyjaśnia dlaczego.
    // Assets wczytuje plik raz - ponowne wejście do sceny używa tej samej czcionki
    std::shared_ptr<const Typeface> font = engine.Assets().Font("font.ttf");
    const bool hasFont = !font->IsDefault();
    scene.ProvideContext<DefaultTypeface>(font);
    const auto title = [&scene](Vector2 position, std::string content) -> visual::Text& {
        return scene.Spawn<visual::Text>()
            .Content(std::move(content))
            .FontSize(18)
            .Paint(DARKGRAY)
            .Position(position);
    };

    // ---------------- animacja bitmap ----------------
    title({40, 30}, "Animacja: LPM - pauza, X - odbicie");
    // jeden opis animacji dla obu sprite'ów: siatka 4 x 2 klatek i nazwane animacje
    const auto wheel =
        std::make_shared<SpriteSheet>(SpriteSheet::Grid(MakeWheelSheet(), {kFrame, kFrame}));
    wheel->Animation("kreci", 0, 8, 12).Animation("waha", 0, 5, 8, Playback::PingPong);
    // każde przejście szprychy przez poziom (klatki 0 i 4) - zdarzenie klatki
    wheel->Event("kreci", 0, "poziomo").Event("kreci", 4, "poziomo");
    auto& turns = title({40, 250}, "obroty: 0").FontSize(16);
    scene.Spawn<visual::Sprite>()
        .Sheet(wheel)
        .Play("kreci")
        .Origin({kFrame / 2, kFrame / 2})
        .Position({100, 140})
        .Scale({2, 2})
        .OnClick([](visual::Sprite& self) {
            if (self.IsPlaying())
                self.Pause();
            else
                self.Play();
        })
        .OnFrameEvent([&turns, count = 0](visual::Sprite&, const std::string&) mutable {
            turns.Content("obroty: " + std::to_string(++count / 2));
        });
    scene.Spawn<visual::Sprite>()
        .Sheet(wheel)
        .Play("waha")
        .Speed(0.5f)
        .Origin({kFrame / 2, kFrame / 2})
        .Position({230, 140})
        .Paint(SKYBLUE)
        .OnKeyPressed(KEY_X, [](visual::Sprite& self) { self.FlippedX(!self.IsFlippedX()); });
    title({40, 230}, "drugie: PingPong, Speed(0.5), Paint(SKYBLUE)").FontSize(16);

    // ---------------- przycinanie ----------------
    title({370, 30}, "ClipRect(): kółko myszy przewija");
    auto& panel = scene.Spawn<Group>().Position({370, 70}).ClipRect({0, 0, 260, 200});
    panel.Spawn<shapes::Rect>().Size({260, 200}).Paint(LIGHTGRAY);
    auto& list = panel.Spawn<Group>();
    for (int i = 0; i < 12; ++i) {
        list.Spawn<shapes::Rect>()
            .Position({10, 10.0f + static_cast<float>(i) * 40})
            .Size({240, 32})
            .Paint(i % 2 ? BEIGE : Color{245, 222, 179, 255})
            .Filled()
            .OnClick([](shapes::Rect& self) { self.Paint(GOLD); })
            .Children([i](shapes::Rect& row) {
                row.Spawn<visual::Text>()
                    .Content("Wiersz " + std::to_string(i + 1) + " (LPM)")
                    .FontSize(18)
                    .Paint(DARKBROWN)
                    .Position({10, 7});
            });
    }
    // zdarzenie na tle panelu: przewijanie listy w granicach
    panel.OnScroll([&list](Group&, float wheelMove) {
        Vector2 position = list.Position();
        position.y = std::clamp(position.y + wheelMove * 30, -290.0f, 0.0f);
        list.Position(position);
    });
    panel.Spawn<shapes::Rect>().Size({260, 200}).Paint(DARKGRAY).Layer(1); // ramka nad listą

    // ---------------- shadery ----------------
    title({690, 30}, "Shader na RenderGroup, G - szarość");
    auto wave = Shared(ShaderProgram::FromCode("", kWaveShader));
    auto gray = Shared(ShaderProgram::FromCode("", kGrayShader));
    if (!wave || !gray) engine.Log(LogLevel::Warning, "shader demo nie dziala");
    scene.Spawn<visual::RenderGroup>()
        .Size({260, 200})
        .Background(::Fade(SKYBLUE, 0.4f))
        .Position({690, 70})
        .Shader(wave)
        .Children([](visual::RenderGroup& surface) {
            surface.Spawn<shapes::Circle>().Position({70, 100}).Radius(50).Paint(RED).Filled();
            surface.Spawn<shapes::Rect>()
                .Position({130, 40})
                .Size({100, 120})
                .Paint(DARKBLUE)
                .Filled();
            for (int x = 0; x < 260; x += 20)
                surface.Spawn<shapes::Line>()
                    .From({static_cast<float>(x), 0})
                    .To({static_cast<float>(x), 200});
        });

    // ---------------- usuwanie obiektów i krok stały ----------------
    title({40, 300}, "Krok stały + Destroy(): LPM usuwa piłkę, B / podwójne kliknięcie dodaje");
    scene.Spawn<shapes::Rect>()
        .Position({kField.x, kField.y})
        .Size({kField.width, kField.height})
        .Paint(DARKGRAY);
    auto& counter = title({40, 330}, "");
    for (int i = 0; i < 6; ++i)
        SpawnBall(scene, {kField.x + 80.0f + static_cast<float>(i) * 140, kField.y + 60});

    // ---------------- tekst ----------------
    title(
        {40, 650},
        hasFont ? "Zażółć gęślą jaźń - czcionka z assets/font.ttf"
                : "Brak assets/font.ttf - font raylib nie ma polskich liter (zamiast nich \"?\")"
    )
        .FontSize(18)
        .Paint(hasFont ? DARKGREEN : MAROON);

    // tekst powiększany Scale() zostaje ostry (atlas w rozmiarze na ekranie), a znaki spoza
    // przygotowanego zestawu (×, ·, €) czcionka dokłada sama przy pierwszym użyciu
    scene.Spawn<visual::Text>()
        .Content("Skala ×2 · 50 €")
        .FontSize(18)
        .Anchor({0.5f, 0.5f})
        .Paint(DARKBLUE)
        .Position({870, 660})
        .OnUpdate([](visual::Text& self, float) {
            const float time = static_cast<float>(Engine::Get().Time());
            const float scale = 1.4f + 0.4f * std::sin(time * 2.0f);
            self.Scale({scale, scale}); // skala bezwzględna (ScaleBy() mnożyłby bieżącą)
        });

    engine.BindAction("nowa pilka", {bind::Key(KEY_B), bind::Gesture(GESTURE_DOUBLETAP)});

    scene.OnFixedUpdate([&scene](float step) {
        // fizyka w kroku stałym: wynik nie zależy od liczby klatek na sekundę
        Balls* balls = scene.Context<Balls>();
        for (Ball* ball : balls->list) {
            ball->velocity.y += 600.0f * step;
            Vector2 p = ball->Position();
            p.x += ball->velocity.x * step;
            p.y += ball->velocity.y * step;
            const float r = ball->Radius();
            if (p.x < kField.x + r || p.x > kField.x + kField.width - r) {
                ball->velocity.x = -ball->velocity.x;
                p.x = std::clamp(p.x, kField.x + r, kField.x + kField.width - r);
            }
            if (p.y > kField.y + kField.height - r) {
                ball->velocity.y = -std::fabs(ball->velocity.y) * 0.9f;
                p.y = kField.y + kField.height - r;
            }
            if (p.y < kField.y + r) {
                ball->velocity.y = std::fabs(ball->velocity.y);
                p.y = kField.y + r;
            }
            ball->Position(p);
        }
    });

    scene.OnUpdate([&scene, &counter, wave, gray](float) {
        Engine& e = Engine::Get();
        if (wave) wave->Uniform("time", static_cast<float>(e.Time()));
        if (e.KeyPressed(KEY_G)) e.PostShader(e.PostShader() ? nullptr : gray);

        if (e.ActionPressed("nowa pilka")) {
            const Vector2 mouse = e.MousePosition();
            const bool inField = ::CheckCollisionPointRec(mouse, kField);
            SpawnBall(scene, inField ? mouse : Vector2{kField.x + kField.width / 2, kField.y + 40});
        }
        const Balls* balls = scene.Context<Balls>();
        counter.Content(
            "Piłki: " + std::to_string(balls->list.size()) +
            "   usunięte: " + std::to_string(balls->removed) +
            "   kroki stałe: " + std::to_string(std::lround(1.0f / e.FixedStep())) +
            "/s   FPS: " + std::to_string(e.CurrentFps())
        );
    });
}
