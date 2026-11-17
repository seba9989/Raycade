/**
 * @file HitScene.cpp
 * @brief Pokaz trafiania myszą: najwyższy obiekt, przepuszczanie, blokowanie, rodzic.
 * @details Trzy kolumny:
 * - karty: przeciągana jest tylko najwyższa karta pod kursorem, a chwycona wychodzi na wierzch,
 * - przepuszczanie i blokowanie: ozdobna ramka bez zdarzeń nie przeszkadza w klikaniu,
 *   a okno z pustym OnClick zatrzymuje kliknięcia w przyciski pod nim,
 * - przekazanie do rodzica: robot złożony z kształtów reaguje jako całość, choć zdarzenia
 *   ma tylko grupa, a nie jej części.
 */

#include <raycade>

#include <format>
#include <string>
#include <utility>

namespace {

/** @brief Najwyższa warstwa użyta przez karty - chwycona karta dostaje następną. */
int topLayer = 0;

/** @brief Prostokątny przycisk z napisem, podświetlany pod kursorem. */
class Button : public Entity {
  public:
    /** @brief Tworzy napis - dziecko bez zdarzeń, więc kliknięcie trafia w przycisk. */
    Button() {
        label_ = &Spawn<node::visual::Text>().FontSize(18).Paint(WHITE).Anchor({0, 0.5f});
        Size(size_);
    }

    template <typename Self> Self& Label(this Self& self, std::string label) {
        self.label_->Content(std::move(label));
        return self;
    }

    /** @brief Ustawia napis sformatowany jak std::format. */
    template <typename Self, typename Arg, typename... Args>
    Self& Label(
        this Self& self,
        std::format_string<Arg, Args...> format,
        Arg&& arg,
        Args&&... args
    ) {
        self.label_->Content(format, std::forward<Arg>(arg), std::forward<Args>(args)...);
        return self;
    }

    template <typename Self> Self& Size(this Self& self, Vector2 size) {
        self.size_ = size;
        self.label_->Position({12, size.y / 2});
        return self;
    }

    bool Contains(Vector2 point) const override {
        const auto local = TryToLocal(point);
        return local && ::CheckCollisionPointRec(*local, {0, 0, size_.x, size_.y});
    }

  protected:
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(IsHovered() ? ::ColorBrightness(color_, 0.3f) : color_)
            .Filled()
            .Rect({0, 0}, size_);
    }

  private:
    node::visual::Text* label_ = nullptr; /**< Napis (dziecko). */
    Vector2 size_{200, 50};               /**< Rozmiar. */
};

/** @brief Karta do przeciągania; chwycona wychodzi na wierzch. */
class Card : public Entity {
  public:
    Card() {
        title_ = &Spawn<node::visual::Text>().FontSize(20).Paint(WHITE).Position({10, 10});
        auto* state = &Spawn<node::visual::Text>().FontSize(16).Paint(WHITE).Position({10, 60});
        OnDragStart([state](Card& self) {
            self.Layer(++topLayer);
            state->Content("przeciągana");
        });
        OnDrag([](Card& self, Vector2 delta) { self.Translate(delta); });
        OnDragEnd([state](Card&) { state->Content(""); });
    }

    template <typename Self> Self& Title(this Self& self, std::string title) {
        self.title_->Content(std::move(title));
        return self;
    }

    bool Contains(Vector2 point) const override {
        const auto local = TryToLocal(point);
        return local && ::CheckCollisionPointRec(*local, {0, 0, kSize.x, kSize.y});
    }

  protected:
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Filled().Rect({0, 0}, kSize);
        // grubsza ramka tylko na karcie, którą naprawdę widać pod kursorem
        if (IsHovered()) {
            renderer.Paint(BLACK).Filled(false);
            for (float i = 0; i < 3; ++i)
                renderer.Rect({-i, -i}, {kSize.x + 2 * i, kSize.y + 2 * i});
        }
    }

  private:
    static constexpr Vector2 kSize{150, 95}; /**< Rozmiar karty. */
    node::visual::Text* title_ = nullptr;    /**< Napis na karcie (dziecko). */
};

/**
 * @brief Robot złożony z kształtów; zdarzenia ma tylko on, nie jego części.
 * @details Kliknięcie w głowę albo tułów trafia najpierw w tę część, a ponieważ część nie ma
 * OnClick ani OnDrag, zdarzenie idzie do rodzica - robota. IsHovered() robota jest prawdą,
 * gdy kursor jest nad którąkolwiek częścią.
 */
class Robot : public node::Group {
  public:
    Robot() {
        body_ = &Spawn<node::shapes::Rect>().Position({-40, 0}).Size({80, 90}).Paint(GRAY).Filled();
        Spawn<node::shapes::Circle>().Position({0, -30}).Radius(28).Paint(LIGHTGRAY).Filled();
        Spawn<node::shapes::Circle>().Position({-10, -34}).Radius(5).Paint(DARKBLUE).Filled();
        Spawn<node::shapes::Circle>().Position({10, -34}).Radius(5).Paint(DARKBLUE).Filled();
        Spawn<node::shapes::Rect>().Position({-60, 10}).Size({18, 60}).Paint(GRAY).Filled();
        Spawn<node::shapes::Rect>().Position({42, 10}).Size({18, 60}).Paint(GRAY).Filled();
        OnDrag([](Robot& self, Vector2 delta) { self.Translate(delta); });
    }

  protected:
    void Tick(float, const Input&) override { body_->Paint(IsHovered() ? ORANGE : GRAY); }

  private:
    node::shapes::Rect* body_ = nullptr; /**< Tułów - zmienia kolor pod kursorem. */
};

} // namespace

/** @brief Konfiguruje scenę pokazu trafiania myszą. */
SCENE("trafienia") {
    // --- kolumna 1: karty (chwyta się tylko najwyższą) ---
    scene.Spawn<Card>().Title("Karta A").Position({40, 110}).Paint(MAROON);
    scene.Spawn<Card>().Title("Karta B").Position({90, 160}).Paint(DARKGREEN);
    scene.Spawn<Card>().Title("Karta C").Position({140, 210}).Paint(DARKBLUE);

    // --- kolumna 2: przepuszczanie i blokowanie ---
    scene.Spawn<Button>()
        .Label("Klik: 0")
        .Position({380, 110})
        .Paint(DARKGREEN)
        .OnClick([count = 0](Button& self) mutable { self.Label("Klik: {}", ++count); });
    // ozdobna ramka bez zdarzeń nad przyciskiem - przepuszcza kliknięcia
    scene.Spawn<node::shapes::Rect>().Position({370, 100}).Size({220, 70}).Paint(GOLD).Layer(1);

    scene.Spawn<Button>()
        .Label("Lewy: 0")
        .Position({380, 260})
        .Paint(DARKBLUE)
        .OnClick([count = 0](Button& self) mutable { self.Label("Lewy: {}", ++count); });
    scene.Spawn<Button>()
        .Label("Prawy: 0")
        .Position({380, 330})
        .Paint(DARKBLUE)
        .OnClick([count = 0](Button& self) mutable { self.Label("Prawy: {}", ++count); });
    // okno nad prawą połową przycisków: puste OnClick blokuje kliknięcia pod spodem
    scene.Spawn<Button>()
        .Label("Okno")
        .Size({140, 170})
        .Position({500, 240})
        .Paint(::Fade(BLACK, 0.6f))
        .Layer(2)
        .OnClick([](Button&) {})
        .OnDrag([](Button& self, Vector2 delta) { self.Translate(delta); });

    // --- kolumna 3: zdarzenia idą do rodzica ---
    scene.Spawn<Robot>().Position({820, 250}).OnClick([&engine](Robot&) {
        engine.Log("Robot klikniety (przez czesc)");
    });

    // ---------------- podpisy ----------------
    const auto label =
        [&scene](Vector2 position, std::string text, float size, Color color = GRAY) {
            scene.Spawn<node::visual::Text>()
                .Content(std::move(text))
                .FontSize(size)
                .Paint(color)
                .Position(position);
        };
    label({30, 20}, "Trafianie myszą: zdarzenie dostaje najwyższy obiekt", 22, DARKGRAY);

    label({40, 70}, "Karty", 20, DARKGRAY);
    label({40, 440}, "przeciągasz tylko górną kartę,\nchwycona wychodzi na wierzch", 16);

    label({370, 70}, "Przepuszczanie i blokowanie", 20, DARKGRAY);
    label({370, 180}, "żółta ramka nie ma zdarzeń - przepuszcza", 16);
    label({370, 430}, "okno ma puste OnClick - blokuje\n(okno można przesuwać)", 16);

    label({690, 70}, "Zdarzenie idzie do rodzica", 20, DARKGRAY);
    label(
        {690, 400},
        "głowa, tułów i ręce nie mają zdarzeń:\nnajechanie i przeciąganie dostaje robot\n"
        "(klik zapisuje się w logu)",
        16
    );
}
