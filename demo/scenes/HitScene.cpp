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

#include <string>

namespace {

/** @brief Najwyższa warstwa użyta przez karty - chwycona karta dostaje następną. */
int topLayer = 0;

/** @brief Prostokątny przycisk z napisem, podświetlany pod kursorem. */
class Button : public Entity {
  public:
    template <typename Self> Self& Label(this Self& self, std::string label) {
        self.label_ = std::move(label);
        return self;
    }

    template <typename Self> Self& Size(this Self& self, Vector2 size) {
        self.size_ = size;
        return self;
    }

    bool Contains(Vector2 point) const override {
        const auto local = TryToLocal(point);
        return local && ::CheckCollisionPointRec(*local, {0, 0, size_.x, size_.y});
    }

  protected:
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(IsHovered() ? ::ColorBrightness(color_, 0.3f) : color_)
            .Fill()
            .Rect({0, 0}, size_);
        ::DrawText(label_.c_str(), 12, static_cast<int>(size_.y / 2) - 9, 18, WHITE);
    }

  private:
    std::string label_;     /**< Napis. */
    Vector2 size_{200, 50}; /**< Rozmiar. */
};

/** @brief Karta do przeciągania; chwycona wychodzi na wierzch. */
class Card : public Entity {
  public:
    Card() {
        OnDragStart([](Card& self) { self.Layer(++topLayer); });
        OnDrag([](Card& self, Vector2 delta) { self.Translate(delta); });
    }

    template <typename Self> Self& Title(this Self& self, std::string title) {
        self.title_ = std::move(title);
        return self;
    }

    bool Contains(Vector2 point) const override {
        const auto local = TryToLocal(point);
        return local && ::CheckCollisionPointRec(*local, {0, 0, kSize.x, kSize.y});
    }

  protected:
    void Draw(PrimitiveRenderer& renderer) const override {
        renderer.Paint(color_).Fill().Rect({0, 0}, kSize);
        // grubsza ramka tylko na karcie, którą naprawdę widać pod kursorem
        if (IsHovered()) {
            renderer.Paint(BLACK).Fill(false);
            for (float i = 0; i < 3; ++i)
                renderer.Rect({-i, -i}, {kSize.x + 2 * i, kSize.y + 2 * i});
        }
        ::DrawText(title_.c_str(), 10, 10, 20, WHITE);
        ::DrawText(IsDragging() ? "przeciagana" : "", 10, 60, 16, WHITE);
    }

  private:
    static constexpr Vector2 kSize{150, 95}; /**< Rozmiar karty. */
    std::string title_;                      /**< Napis na karcie. */
};

/**
 * @brief Robot złożony z kształtów; zdarzenia ma tylko on, nie jego części.
 * @details Kliknięcie w głowę albo tułów trafia najpierw w tę część, a ponieważ część nie ma
 * OnClick ani OnDrag, zdarzenie idzie do rodzica - robota. IsHovered() robota jest prawdą,
 * gdy kursor jest nad którąkolwiek częścią.
 */
class Robot : public shapes::Group {
  public:
    Robot() {
        body_ = &Spawn<shapes::Rect>().Position({-40, 0}).Size({80, 90}).Paint(GRAY).Fill();
        Spawn<shapes::Circle>().Position({0, -30}).Radius(28).Paint(LIGHTGRAY).Fill();
        Spawn<shapes::Circle>().Position({-10, -34}).Radius(5).Paint(DARKBLUE).Fill();
        Spawn<shapes::Circle>().Position({10, -34}).Radius(5).Paint(DARKBLUE).Fill();
        Spawn<shapes::Rect>().Position({-60, 10}).Size({18, 60}).Paint(GRAY).Fill();
        Spawn<shapes::Rect>().Position({42, 10}).Size({18, 60}).Paint(GRAY).Fill();
        OnDrag([](Robot& self, Vector2 delta) { self.Translate(delta); });
    }

  protected:
    void Tick(float, const Input&) override { body_->Paint(IsHovered() ? ORANGE : GRAY); }

  private:
    shapes::Rect* body_ = nullptr; /**< Tułów - zmienia kolor pod kursorem. */
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
        .OnClick([count = 0](Button& self) mutable {
            self.Label("Klik: " + std::to_string(++count));
        });
    // ozdobna ramka bez zdarzeń nad przyciskiem - przepuszcza kliknięcia
    scene.Spawn<shapes::Rect>().Position({370, 100}).Size({220, 70}).Paint(GOLD).Layer(1);

    scene.Spawn<Button>()
        .Label("Lewy: 0")
        .Position({380, 260})
        .Paint(DARKBLUE)
        .OnClick([count = 0](Button& self) mutable {
            self.Label("Lewy: " + std::to_string(++count));
        });
    scene.Spawn<Button>()
        .Label("Prawy: 0")
        .Position({380, 330})
        .Paint(DARKBLUE)
        .OnClick([count = 0](Button& self) mutable {
            self.Label("Prawy: " + std::to_string(++count));
        });
    // okno nad prawą połową przycisków: puste OnClick blokuje kliknięcia pod spodem
    scene.Spawn<Button>()
        .Label("Okno")
        .Size({140, 170})
        .Position({500, 240})
        .Paint(Fade(BLACK, 0.6f))
        .Layer(2)
        .OnClick([](Button&) {})
        .OnDrag([](Button& self, Vector2 delta) { self.Translate(delta); });

    // --- kolumna 3: zdarzenia idą do rodzica ---
    scene.Spawn<Robot>().Position({820, 250}).OnClick([](Robot&) {
        Engine::Get().Log("Robot klikniety (przez czesc)");
    });

    scene.OnDraw([](PrimitiveRenderer&) {
        ::DrawText("Trafianie mysza: zdarzenie dostaje najwyzszy obiekt", 30, 20, 22, DARKGRAY);

        ::DrawText("Karty", 40, 70, 20, DARKGRAY);
        ::DrawText("przeciagasz tylko gorna karte,", 40, 440, 16, GRAY);
        ::DrawText("chwycona wychodzi na wierzch", 40, 460, 16, GRAY);

        ::DrawText("Przepuszczanie i blokowanie", 370, 70, 20, DARKGRAY);
        ::DrawText("zolta ramka nie ma zdarzen - przepuszcza", 370, 180, 16, GRAY);
        ::DrawText("okno ma puste OnClick - blokuje", 370, 430, 16, GRAY);
        ::DrawText("(okno mozna przesuwac)", 370, 450, 16, GRAY);

        ::DrawText("Zdarzenie idzie do rodzica", 690, 70, 20, DARKGRAY);
        ::DrawText("glowa, tulow i rece nie maja zdarzen:", 690, 400, 16, GRAY);
        ::DrawText("najechanie i przeciaganie dostaje robot", 690, 420, 16, GRAY);
        ::DrawText("(klik zapisuje sie w logu)", 690, 440, 16, GRAY);
    });
}
