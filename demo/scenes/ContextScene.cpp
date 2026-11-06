/**
 * @file ContextScene.cpp
 * @brief Pokaz kontekstu (Contextable): poziomy obiektu, sceny i silnika.
 * @details Scena pokazuje kolejno:
 * - kontekst poddrzewa: panel udostępnia Theme swoim kafelkom (ProvideContext w konstruktorze),
 *   a scena nadpisuje go po Spawn; kafelek poza panelem dostaje Theme sceny,
 * - wspólny stan: kliknięcie kafelka zmienia Theme całego panelu i podbija wynik sceny,
 * - SetContext: scena udostępnia gracza przez referencję, a pościgowcy znajdują go
 *   przez GetContext<Player>(),
 * - poziom silnika: rekord przetrwa ReloadScene (R), a wynik sceny wraca do zera.
 *
 * Klasy komponentów są zdefiniowane lokalnie w tym pliku - są potrzebne tylko tej scenie.
 */

#include <raycade>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace {

/** @brief Kolory, między którymi przełącza się Theme. */
constexpr std::array<Color, 5> kPalette{SKYBLUE, ORANGE, LIME, VIOLET, GOLD};

/** @brief Motyw udostępniany poddrzewu albo całej scenie. */
struct Theme {
    int color = 0;              /**< Indeks w kPalette. */
    const char* name = "scena"; /**< Kto udostępnił motyw (do podpisu). */
};

/** @brief Wynik bieżącej rozgrywki - kontekst sceny, zeruje się przy ReloadScene. */
struct Score {
    int value = 0;
};

/** @brief Rekord - kontekst silnika, przetrwa zwolnienie i ponowne wczytanie sceny. */
struct BestScore {
    int value = 0;
};

/**
 * @brief Kafelek w kolorze najbliższego Theme.
 * @details Wskaźnik do Theme zapamiętuje w OnMount() - konstruktor nie zna jeszcze rodzica.
 * Ponowne ProvideContext<Theme>() przypisuje wartość w tym samym miejscu, więc wskaźnik
 * pozostaje ważny i od razu widzi zmiany.
 */
class Tile : public Entity {
  public:
    bool Contains(Vector2 point) const override {
        const auto local = TryToLocal(point);
        return local && ::CheckCollisionPointRec(*local, {0, 0, kSize, kSize});
    }

  protected:
    void OnMount() override { theme_ = GetContext<Theme>(); }

    void Tick(float, const Input& input) override {
        if (!Hovered() || !input.MousePressed(MOUSE_BUTTON_LEFT) || !theme_) return;
        // zmiana wspólnego Theme - przemalowują się wszystkie kafelki, które go widzą
        theme_->color = (theme_->color + 1) % static_cast<int>(kPalette.size());
        if (Score* score = GetContext<Score>()) {
            ++score->value;
            if (BestScore* best = GetContext<BestScore>())
                best->value = std::max(best->value, score->value);
        }
    }

    void Draw(PrimitiveRenderer& renderer) const override {
        const Color color = theme_ ? kPalette[static_cast<std::size_t>(theme_->color)] : GRAY;
        renderer.Paint(color).Fill().Rect({0, 0}, {kSize, kSize});
        if (Hovered()) renderer.Paint(BLACK).Fill(false).Rect({-2, -2}, {kSize + 4, kSize + 4});
    }

  private:
    static constexpr float kSize = 60.0f; /**< Bok kafelka. */
    Theme* theme_ = nullptr;              /**< Najbliższy Theme, ustalony w OnMount(). */
};

/**
 * @brief Panel z trzema kafelkami, który udostępnia im własny Theme.
 * @details Domyślny Theme powstaje w konstruktorze; scena może go nadpisać po Spawn.
 */
class Panel : public shapes::Group {
  public:
    Panel() {
        ProvideContext<Theme>(Theme{0, "panel (domyslny)"});
        for (int i = 0; i < 3; ++i)
            Spawn<Tile>().Position({static_cast<float>(i) * 75.0f, 0});
    }
};

/** @brief Gracz sterowany strzałkami w obrębie areny. */
class Player : public shapes::Circle {
  public:
    static constexpr Rectangle kArena{540, 110, 420, 300}; /**< Obszar ruchu. */

  protected:
    void Tick(float dt, const Input& input) override {
        Vector2 position = Position();
        const float step = 250.0f * dt;
        if (input.KeyDown(KEY_RIGHT)) position.x += step;
        if (input.KeyDown(KEY_LEFT)) position.x -= step;
        if (input.KeyDown(KEY_DOWN)) position.y += step;
        if (input.KeyDown(KEY_UP)) position.y -= step;
        position.x = std::clamp(position.x, kArena.x, kArena.x + kArena.width);
        position.y = std::clamp(position.y, kArena.y, kArena.y + kArena.height);
        Position(position);
    }
};

/** @brief Kółko goniące gracza znalezionego przez GetContext<Player>(). */
class Follower : public shapes::Circle {
  public:
    template <typename Self> Self& Speed(this Self& self, float speed) {
        self.speed_ = speed;
        return self;
    }

  protected:
    void Tick(float dt, const Input&) override {
        const Player* player = GetContext<Player>();
        if (!player) return;
        const Vector2 from = Position();
        const Vector2 to = player->Position();
        const float dx = to.x - from.x;
        const float dy = to.y - from.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance < 30.0f) return; // nie wchodzimy na gracza
        const float step = std::min(speed_ * dt, distance);
        Translate({dx / distance * step, dy / distance * step});
    }

  private:
    float speed_ = 80.0f; /**< Prędkość w pikselach na sekundę. */
};

/** @brief Pasek wyniku sceny ze znacznikiem rekordu z kontekstu silnika. */
class ScoreBar : public Entity {
  protected:
    void Draw(PrimitiveRenderer& renderer) const override {
        constexpr float kWidth = 420.0f;
        constexpr float kMax = 20.0f; // tyle kliknięć wypełnia pasek
        const auto fraction = [&](int value) { return std::min(value / kMax, 1.0f) * kWidth; };

        renderer.Paint(LIGHTGRAY).Fill().Rect({0, 0}, {kWidth, 24});
        if (const Score* score = GetContext<Score>())
            renderer.Paint(DARKGREEN).Fill().Rect({0, 0}, {fraction(score->value), 24});
        if (const BestScore* best = GetContext<BestScore>()) {
            const float x = fraction(best->value);
            renderer.Paint(MAROON).Fill().Rect({x - 2, -6}, {4, 36});
        }
    }
};

} // namespace

/** @brief Konfiguruje scenę pokazu kontekstu. */
SCENE("kontekst") {
    Engine& engine = Engine::Get();

    // Poziom silnika: zwykle udostępnia się go w main(); tu tylko wtedy, gdy jeszcze go nie ma,
    // żeby rekord przetrwał ReloadScene (konfiguracja sceny wykonuje się wtedy ponownie).
    if (!engine.GetContext<BestScore>()) engine.ProvideContext<BestScore>();

    // Poziom sceny: widzą go wszystkie obiekty sceny. Theme sceny musi powstać przed kafelkiem,
    // który go używa, bo kafelek odczytuje kontekst w OnMount() - zaraz po Spawn.
    scene.ProvideContext<Score>().ProvideContext<Theme>(Theme{4, "scena"});

    // Poziom poddrzewa: każdy panel ma własny Theme; scena nadpisuje domyślny po Spawn.
    scene.Spawn<Panel>().Position({60, 110}).ProvideContext<Theme>(Theme{0, "panel A"});
    scene.Spawn<Panel>().Position({60, 230}).ProvideContext<Theme>(Theme{1, "panel B"});
    scene.Spawn<Tile>().Position({60, 350}); // bez panelu - widzi Theme sceny

    // SetContext: gracza udostępniamy przez referencję, bez kopiowania.
    auto& player = scene.Spawn<Player>();
    player.Position({750, 260}).Radius(16).Paint(DARKBLUE).Fill();
    scene.SetContext(player);

    // Radius() z shapes::Circle zwraca Circle&, dlatego jest na końcu łańcucha
    scene.Spawn<Follower>().Speed(60).Position({560, 130}).Paint(RED).Fill().Radius(10);
    scene.Spawn<Follower>().Speed(90).Position({940, 130}).Paint(MAROON).Fill().Radius(10);
    scene.Spawn<Follower>().Speed(120).Position({940, 390}).Paint(PINK).Fill().Radius(10);

    scene.Spawn<ScoreBar>().Position({60, 520});

    scene.OnUpdate([&engine](float) {
        if (engine.KeyPressed(KEY_R)) engine.ReloadScene("kontekst");
    });

    scene.OnDraw([&scene](PrimitiveRenderer&) {
        ::DrawText("Kontekst poddrzewa: panel -> kafelki", 60, 70, 20, DARKGRAY);
        ::DrawText("panel A", 290, 130, 18, GRAY);
        ::DrawText("panel B", 290, 250, 18, GRAY);
        ::DrawText("kafelek bez panelu: Theme sceny", 140, 370, 18, GRAY);
        ::DrawText("LPM na kafelku - zmiana Theme calego panelu", 60, 430, 16, GRAY);

        ::DrawText("SetContext(gracz): poscigowcy znajduja gracza", 540, 70, 20, DARKGRAY);
        ::DrawRectangleLinesEx(Player::kArena, 1.0f, LIGHTGRAY);
        ::DrawText("strzalki - ruch gracza", 540, 420, 16, GRAY);

        const Score* score = scene.GetContext<Score>();
        const BestScore* best = scene.GetContext<BestScore>(); // scena -> silnik
        const std::string text = "Wynik (scena): " + std::to_string(score ? score->value : 0) +
                                 "   Rekord (silnik): " + std::to_string(best ? best->value : 0);
        ::DrawText(text.c_str(), 60, 490, 20, DARKGRAY);
        ::DrawText("R - ReloadScene: wynik sceny od zera, rekord zostaje", 60, 560, 16, GRAY);
    });
}
