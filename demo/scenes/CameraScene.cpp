/**
 * @file CameraScene.cpp
 * @brief Pokaz kamery 2D: podążanie, przybliżanie, obrót, obiekty świata i ekranu (HUD).
 * @details
 * - gracz (WASD/strzałki) chodzi po świecie, a kamera płynnie za nim podąża,
 * - kółko myszy przybliża, Q/E obracają widok, F włącza i wyłącza podążanie, R resetuje,
 * - skrzynie można przeciągać, a monety klikać - trafienia działają przy każdym przybliżeniu
 *   i obrocie, bo scena przelicza kursor przez kamerę,
 * - przyciski w rogu są obiektami ekranu (ScreenSpace()): kamera ich nie przesuwa i leżą
 *   nad światem, więc kliknięcie w nie nie trafia w świat pod spodem,
 * - siatka świata jest rysowana w OnDrawWorld(), a napisy w OnDraw() (układ okna).
 */

#include <raycade>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace {

/** @brief Połowa boku kwadratowego świata. */
constexpr float kWorld = 1200.0f;

/** @brief Liczba zebranych monet (pokazywana w HUD). */
int coins = 0;

/** @brief Gracz sterowany WASD/strzałkami, ograniczony do świata. */
class Walker : public shapes::Circle {
  protected:
    void Tick(float dt, const Input& input) override {
        Vector2 direction{};
        if (input.KeyDown(KEY_A) || input.KeyDown(KEY_LEFT)) direction.x -= 1;
        if (input.KeyDown(KEY_D) || input.KeyDown(KEY_RIGHT)) direction.x += 1;
        if (input.KeyDown(KEY_W) || input.KeyDown(KEY_UP)) direction.y -= 1;
        if (input.KeyDown(KEY_S) || input.KeyDown(KEY_DOWN)) direction.y += 1;
        const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length == 0.0f) return;
        Vector2 position = Position();
        position.x += direction.x / length * 300.0f * dt;
        position.y += direction.y / length * 300.0f * dt;
        position.x = std::clamp(position.x, -kWorld, kWorld);
        position.y = std::clamp(position.y, -kWorld, kWorld);
        Position(position);
    }
};

/** @brief Prostokątny przycisk HUD z napisem. */
class HudButton : public Entity {
  public:
    template <typename Self> Self& Label(this Self& self, std::string label) {
        self.label_ = std::move(label);
        return self;
    }

    bool Contains(Vector2 point) const override {
        const auto local = TryToLocal(point);
        return local && ::CheckCollisionPointRec(*local, {0, 0, kSize.x, kSize.y});
    }

  protected:
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(Hovered() ? ::ColorBrightness(color_, 0.3f) : color_)
            .Fill()
            .Rect({0, 0}, kSize);
        ::DrawText(label_.c_str(), 10, 8, 20, WHITE);
    }

  private:
    static constexpr Vector2 kSize{110, 36}; /**< Rozmiar przycisku. */
    std::string label_;                      /**< Napis. */
};

/** @brief Skrzynia do przeciągania (z ramką pod kursorem). */
class Crate : public shapes::Rect {
  public:
    Crate() {
        Size({60, 60}).Paint(BROWN).Fill();
        OnDrag([](Crate& self, Vector2 delta) { self.Translate(delta); });
    }

  protected:
    void Draw(PrimitiveRenderer& renderer) const override {
        shapes::Rect::Draw(renderer);
        if (Hovered()) renderer.Paint(BLACK).Fill(false).Rect({-3, -3}, {66, 66});
    }
};

/** @brief Moneta: kliknięta przenosi się w losowe miejsce świata. */
class Coin : public shapes::Circle {
  public:
    Coin() {
        Radius(14).Paint(GOLD).Fill();
        OnClick([](Coin& self) {
            ++coins;
            Engine& engine = Engine::Get();
            self.Position(
                {engine.RandomFloat(-kWorld, kWorld), engine.RandomFloat(-kWorld, kWorld)});
        });
    }
};

/** @brief Ustawia kamerę w stanie początkowym. */
void ResetCamera(SceneCamera& camera, const Walker& player) {
    camera.Offset({500, 330}).Zoom(1.0f).Rotation(0.0f).Follow(player, 4.0f);
}

} // namespace

/** @brief Konfiguruje scenę pokazu kamery. */
SCENE("kamera") {
    Engine& engine = Engine::Get();

    // --- świat ---
    for (int i = 0; i < 8; ++i) {
        const float angle = static_cast<float>(i) * PI / 4.0f;
        scene.Spawn<Crate>().Position(
            {std::cos(angle) * 350.0f - 30, std::sin(angle) * 350.0f - 30});
    }
    for (int i = 0; i < 12; ++i)
        scene.Spawn<Coin>().Position(
            {engine.RandomFloat(-kWorld, kWorld), engine.RandomFloat(-kWorld, kWorld)});

    auto& player = scene.Spawn<Walker>();
    player.Position({0, 0}).Paint(DARKBLUE).Fill().Radius(20);
    ResetCamera(scene.GetCamera(), player);

    // --- HUD: obiekty ekranu, kamera ich nie przesuwa ---
    SceneCamera& camera = scene.GetCamera();
    scene.Spawn<HudButton>()
        .ScreenSpace()
        .Label("Zoom +")
        .Position({870, 20})
        .Paint(DARKGREEN)
        .OnClick([&camera](HudButton&) { camera.Zoom(std::min(camera.Zoom() * 1.25f, 4.0f)); });
    scene.Spawn<HudButton>()
        .ScreenSpace()
        .Label("Zoom -")
        .Position({870, 64})
        .Paint(DARKGREEN)
        .OnClick([&camera](HudButton&) { camera.Zoom(std::max(camera.Zoom() / 1.25f, 0.25f)); });
    scene.Spawn<HudButton>()
        .ScreenSpace()
        .Label("Reset")
        .Position({870, 108})
        .Paint(MAROON)
        .OnClick([&camera, &player](HudButton&) { ResetCamera(camera, player); });

    // --- sterowanie kamerą ---
    scene.OnUpdate([&engine, &camera, &player](float dt) {
        const Input& input = engine.GetInput();
        if (const float wheel = input.MouseWheel(); wheel != 0.0f)
            camera.Zoom(std::clamp(camera.Zoom() * std::pow(1.15f, wheel), 0.25f, 4.0f));
        if (input.KeyDown(KEY_Q)) camera.Rotation(camera.Rotation() - 1.5f * dt);
        if (input.KeyDown(KEY_E)) camera.Rotation(camera.Rotation() + 1.5f * dt);
        if (input.KeyPressed(KEY_F)) {
            if (camera.Following())
                camera.StopFollowing();
            else
                camera.Follow(player, 4.0f);
        }
        if (input.KeyPressed(KEY_R)) ResetCamera(camera, player);
    });

    // --- siatka świata: przesuwa się, przybliża i obraca razem ze światem ---
    scene.OnDrawWorld([](PrimitiveRenderer&) {
        for (float x = -kWorld; x <= kWorld; x += 100.0f)
            ::DrawLineV({x, -kWorld}, {x, kWorld}, x == 0 ? GRAY : LIGHTGRAY);
        for (float y = -kWorld; y <= kWorld; y += 100.0f)
            ::DrawLineV({-kWorld, y}, {kWorld, y}, y == 0 ? GRAY : LIGHTGRAY);
        ::DrawRectangleLinesEx({-kWorld, -kWorld, 2 * kWorld, 2 * kWorld}, 4.0f, DARKGRAY);
    });

    // --- napisy w układzie okna ---
    scene.OnDraw([&engine, &camera](PrimitiveRenderer&) {
        const Vector2 mouse = camera.ScreenToWorld(engine.MousePosition());
        char info[160];
        std::snprintf(info, sizeof info,
                      "zoom %.2f   obrot %.0f st.   podazanie: %s   kursor w swiecie: (%.0f, %.0f)",
                      camera.Zoom(), camera.Rotation() * RAD2DEG,
                      camera.Following() ? "tak" : "nie", mouse.x, mouse.y);
        ::DrawRectangle(0, 0, 850, 70, Fade(RAYWHITE, 0.85f));
        ::DrawText("Kamera 2D", 20, 12, 22, DARKGRAY);
        ::DrawText(info, 20, 42, 16, DARKGRAY);
        ::DrawText(("Monety: " + std::to_string(coins)).c_str(), 870, 160, 20, DARKGRAY);
        ::DrawText("WASD/strzalki - ruch   kolko - zoom   Q/E - obrot   F - podazanie   R - reset",
                   20, 618, 16, DARKGRAY);
        ::DrawText("przeciagnij skrzynie, kliknij monete - trafienia dzialaja przy kazdym zoomie",
                   20, 640, 16, DARKGRAY);
    });
}
