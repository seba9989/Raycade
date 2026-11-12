/**
 * @file PhysicsScene.cpp
 * @brief Pokaz kolizji: Collider\<S\>, StaticBody, CharacterBody z MoveAndSlide() i Area.
 * @details
 * - postać (CharacterBody) chodzi A/D albo strzałkami i skacze spacją, W albo strzałką w górę;
 *   ruch liczy MoveAndSlide() w kroku stałym (FixedTick()),
 * - podłoże to collidery o różnych kształtach z @ref shapes - prostokąty, trójkątna pochylnia,
 *   koło i obrócona platforma - wszystkie widoczne (Visible()), więc są też grafiką,
 * - ruchoma platforma to StaticBody przesuwany w OnFixedUpdate(),
 * - szary blok zmienia kolor w OnCollisionEnter() / OnCollisionExit(),
 * - monety i lawa to obszary (Area) z OnBodyEnter(),
 * - brązowa półka to platforma jednostronna (OneWay()): wskakuje się na nią od dołu, a S albo
 *   strzałka w dół na chwilę wyłącza jej collider (Disabled()) - postać spada przez nią,
 * - brązowe skrzynie i piłki to bryły sztywne (RigidBody): spadają, odbijają się, obracają
 *   i układają w stos; gracz pcha skrzynie siłą przyłożoną przy zderzeniu (SlideCollisions()),
 *   a B tworzy piłkę pod kursorem,
 * - fioletowe oko w rogu co klatkę rzuca promień do gracza (Scene::Raycast()): zielona linia -
 *   widzi go, czerwona - kończy się na przeszkodzie,
 * - F1 pokazuje obrysy wszystkich colliderów (Scene::DebugColliders()), R wczytuje scenę od nowa.
 */

#include <raycade>

#include <cmath>
#include <string>

namespace {

/** @brief Liczba zebranych monet. */
int coins = 0;

/** @brief Liczba monet na scenie. */
constexpr int kCoins = 5;

/** @brief Postać gracza: prostokąt z colliderem tego samego kształtu. */
class Player : public CharacterBody {
  public:
    /** @brief Buduje grafikę i collider (punkt zaczepienia - środek dolnej krawędzi). */
    Player() {
        body_ = &Spawn<shapes::Rect>().Position({-12, -32}).Size({24, 32}).Paint(BLUE).Filled();
        Spawn<collider::Rect>().Position({-12, -32}).Size({24, 32});
    }

    /** @brief Wraca na start (lawa, upadek poza scenę). */
    void Respawn() {
        Position(kSpawn);
        Velocity({0, 0});
    }

  protected:
    /** @brief Jednorazowe naciśnięcia sprawdzane co klatkę - w kroku stałym mogłyby zginąć. */
    void Tick(float, const Input& input) override {
        if (input.KeyPressed(KEY_SPACE) || input.KeyPressed(KEY_W) || input.KeyPressed(KEY_UP))
            jump_ = true;
        body_->Paint(IsOnFloor() ? BLUE : SKYBLUE);
    }

    /** @brief Ruch w kroku stałym: sterowanie, grawitacja, skok i MoveAndSlide(). */
    void FixedTick(float step, const Input& input) override {
        const float right = input.KeyDown(KEY_D) || input.KeyDown(KEY_RIGHT) ? 1.0f : 0.0f;
        const float left = input.KeyDown(KEY_A) || input.KeyDown(KEY_LEFT) ? 1.0f : 0.0f;
        Vector2 velocity = Velocity();
        velocity.x = (right - left) * 260.0f;
        velocity.y += Gravity().y * step;
        if (jump_ && IsOnFloor()) velocity.y = -540.0f;
        jump_ = false;
        Velocity(velocity).MoveAndSlide();
        // pchanie skrzyń: postać ma dla brył nieskończoną masę, więc pcha je sama - siłą w bok
        for (const Collision& hit : SlideCollisions())
            if (auto* crate = dynamic_cast<RigidBody*>(hit.other))
                crate->ApplyCentralForce({-hit.normal.x * 2500.0f, 0.0f});
        if (Position().y > 900) Respawn();
    }

  private:
    static constexpr Vector2 kSpawn{60, 560}; /**< Punkt startowy. */
    shapes::Rect* body_ = nullptr;            /**< Grafika (kolor zależy od IsOnFloor()). */
    bool jump_ = false;                       /**< Skok zgłoszony w Tick(). */
};

/** @brief Dodaje nieruchomy prostokąt (widoczny collider). */
StaticBody& Block(Scene& scene, Vector2 position, Vector2 size, Color color) {
    StaticBody& body = scene.Spawn<StaticBody>().Position(position);
    body.Spawn<collider::Rect>().Size(size).Paint(color).Filled().Visible();
    return body;
}

/** @brief Dodaje skrzynię - bryłę sztywną, którą gracz może pchać. */
void Crate(Scene& scene, Vector2 center) {
    auto& crate = scene.Spawn<RigidBody>().Position(center).Mass(1).Friction(0.7f);
    crate.Spawn<collider::Rect>()
        .Position({-15, -15})
        .Size({30, 30})
        .Paint(BROWN)
        .Filled()
        .Visible();
    crate.Spawn<shapes::Rect>().Position({-15, -15}).Size({30, 30}).Paint(DARKBROWN);
}

/** @brief Dodaje sprężystą piłkę - bryłę sztywną w kształcie koła. */
void Ball(Scene& scene, Vector2 center) {
    auto& ball = scene.Spawn<RigidBody>().Position(center).Mass(0.5f).Bounce(0.6f).Friction(0.4f);
    ball.Spawn<collider::Circle>().Radius(10).Paint(MAROON).Filled().Visible();
    ball.Spawn<shapes::Line>().From({0, 0}).To({10, 0}).Paint(WHITE); // widać obrót
}

/** @brief Dodaje monetę - obszar, który znika po wejściu gracza. */
void Coin(Scene& scene, Vector2 position) {
    scene.Spawn<Area>()
        .Position(position)
        .Children([](Area& coin) {
            coin.Spawn<collider::Circle>().Radius(10).Paint(GOLD).Filled().Visible();
        })
        .OnBodyEnter([](Area& coin, PhysicsBody& body) {
            if (!dynamic_cast<Player*>(&body)) return;
            ++coins;
            coin.Destroy();
        });
}

} // namespace

/** @brief Konfiguruje scenę kolizji. */
SCENE("fizyka") {
    coins = 0;
    scene.Gravity({0, 1400});

    // podłoga i niewidoczne ściany na brzegach okna (widać je po F1)
    Block(scene, {0, 620}, {1000, 80}, DARKGRAY);
    scene.Spawn<StaticBody>().Position({-20, 0}).Children([](StaticBody& wall) {
        wall.Spawn<collider::Rect>().Size({20, 700});
    });
    scene.Spawn<StaticBody>().Position({1000, 0}).Children([](StaticBody& wall) {
        wall.Spawn<collider::Rect>().Size({20, 700});
    });

    // różne kształty colliderów jako podłoże
    Block(scene, {120, 480}, {180, 20}, GRAY);
    scene.Spawn<StaticBody>().Children([](StaticBody& hill) {
        // pochylnia ~34° - łagodniejsza niż FloorMaxAngle(), więc to podłoga, a nie ściana
        hill.Spawn<collider::Triangle>()
            .Vertices({380, 620}, {560, 620}, {560, 500})
            .Paint(GRAY)
            .Filled()
            .Visible();
    });
    Block(scene, {560, 500}, {160, 120}, GRAY);
    scene.Spawn<StaticBody>().Position({300, 620}).Children([](StaticBody& bump) {
        bump.Spawn<collider::Circle>().Radius(40).Paint(GRAY).Filled().Visible();
    });
    // obrócona platforma - collider obraca się razem z ciałem
    scene.Spawn<StaticBody>().Position({830, 420}).Rotate(-0.3f).Children([](StaticBody& tilt) {
        tilt.Spawn<collider::Rect>()
            .Position({-70, -10})
            .Size({140, 20})
            .Paint(GRAY)
            .Filled()
            .Visible();
    });

    // ruchoma platforma: StaticBody przesuwany w kodzie
    Block(scene, {420, 330}, {120, 16}, DARKBLUE)
        .OnFixedUpdate([time = 0.0f](StaticBody& self, float step) mutable {
            time += step;
            self.Position({420 + 140 * std::sin(time), 330});
        });

    // platforma jednostronna nad blokiem - wskakuje się na nią od dołu
    auto& ledge = scene.Spawn<StaticBody>().Position({580, 410}).Spawn<collider::Rect>();
    ledge.Size({140, 8}).OneWay().Paint(BROWN).Filled().Visible();

    // blok reagujący na zderzenia (OnCollisionEnter / OnCollisionExit)
    StaticBody& touch = scene.Spawn<StaticBody>().Position({140, 330});
    auto& tile = touch.Spawn<collider::Rect>().Size({90, 20}).Paint(GRAY).Filled().Visible();
    touch.OnCollisionEnter(
             [&tile](StaticBody&, const Collision&) { tile.Paint(ORANGE); }
    ).OnCollisionExit([&tile](StaticBody&, PhysicsBody&) { tile.Paint(GRAY); });

    // obszary: monety i lawa
    for (const Vector2 position :
         {Vector2{210, 450}, Vector2{640, 470}, Vector2{830, 370}, Vector2{480, 290},
          Vector2{185, 295}})
        Coin(scene, position);
    scene.Spawn<Area>()
        .Position({880, 604})
        .Children([](Area& lava) {
            lava.Spawn<collider::Rect>().Size({100, 16}).Paint(RED).Filled().Visible();
        })
        .OnBodyEnter([](Area&, PhysicsBody& body) {
            if (auto* player = dynamic_cast<Player*>(&body)) player->Respawn();
        });

    // bryły sztywne: stos skrzyń przy lawie i piłka na platformie
    for (int i = 0; i < 3; ++i)
        Crate(scene, {800, 604.0f - 31.0f * static_cast<float>(i)});
    Crate(scene, {740, 604});
    Ball(scene, {250, 300});

    Player& player = scene.Spawn<Player>();
    player.Respawn();

    // oko: promień do gracza - czy nic nie zasłania widoku
    constexpr Vector2 kEye{960, 90};
    scene.Spawn<shapes::Circle>().Position(kEye).Radius(10).Paint(DARKPURPLE).Filled();
    scene.OnDrawWorld([&scene, &player, kEye](PrimitiveRenderer& renderer) {
        const Vector2 target{player.WorldPosition().x, player.WorldPosition().y - 16};
        const auto hit = scene.Raycast(kEye, target);
        const bool sees = hit && hit->body == &player;
        renderer.Paint(sees ? GREEN : RED).Line(kEye, hit ? hit->point : target);
        if (hit && !sees) renderer.Circle(hit->point, 3);
    });

    scene.OnUpdate([&scene, &ledge](float) {
        Engine& engine = Engine::Get();
        if (engine.KeyPressed(KEY_F1)) scene.DebugColliders(!scene.IsDebugColliders());
        if (engine.KeyPressed(KEY_R)) engine.ReloadScene("fizyka");
        if (engine.KeyPressed(KEY_B)) Ball(scene, engine.MousePosition());
        // zeskok przez platformę jednostronną: collider wyłączony, póki trzymany jest klawisz
        ledge.Disabled(engine.KeyDown(KEY_S) || engine.KeyDown(KEY_DOWN));
    });

    scene.OnDraw([&player](PrimitiveRenderer&) {
        const auto yes = [](bool value) { return value ? "tak" : "nie"; };
        ::DrawText(
            "A/D lub strzalki - ruch   spacja/W - skok   S - zeskok z polki   B - pilka pod "
            "kursorem   \n"
            "F1 - collidery   R - od nowa",
            20, 20, 18, DARKGRAY
        );
        const std::string state =
            "monety: " + std::to_string(coins) + "/" + std::to_string(kCoins) +
            "   podloga: " + yes(player.IsOnFloor()) + "   sciana: " + yes(player.IsOnWall()) +
            "   sufit: " + yes(player.IsOnCeiling());
        ::DrawText(state.c_str(), 20, 46, 18, GRAY);
        if (coins == kCoins) ::DrawText("Wszystkie monety zebrane!", 360, 120, 28, GOLD);
    });
}
