/**
 * @file PrimitivesScene.cpp
 * @brief Demonstracja figur, łamanych, warstw, dzieci i algorytmu odcinka (lab 2).
 * @details Scena pokazuje interakcję myszy i klawiatury, porównanie algorytmu
 * przyrostowego z funkcją biblioteki raylib oraz klasy Point2D i LineSegment.
 */

#include <cmath>
#include <vector>

#include "lib/core/Engine.hpp"

/** @brief Konfiguruje interaktywną scenę prymitywów. */
SCENE("prymitywy") {
    // prostokąt sterowany strzałkami, L przenosi go nad / pod błękitny
    scene.Spawn<shapes::Rect>()
        .Position({40, 60})
        .Size({120, 80})
        .Paint(DARKBLUE)
        .Fill()
        .OnUpdate([](shapes::Rect& self, float dt) {
            const Engine& engine = Engine::Get();
            Vector2 position = self.Position();
            const float speed = 200 * dt;
            if (engine.KeyDown(KEY_RIGHT)) position.x += speed;
            if (engine.KeyDown(KEY_LEFT)) position.x -= speed;
            if (engine.KeyDown(KEY_DOWN)) position.y += speed;
            if (engine.KeyDown(KEY_UP)) position.y -= speed;
            self.Position(position);
        })
        .OnKeyPressed(KEY_L, [](shapes::Rect& self) { self.Layer(self.Layer() == 0 ? 1 : 0); })
        // dzieci jadą razem z prostokątem - ich pozycje są względem niego
        .Children([](shapes::Rect& self) {
            self.Spawn<shapes::Circle>().Position({30, 30}).Radius(10).Paint(WHITE).Fill();
            self.Spawn<shapes::Rect>()
                .Position({70, 45})
                .Size({40, 25})
                .Paint(YELLOW)
                .Fill()
                .Children([](shapes::Rect& self) { // wnuk: względem żółtego prostokąta
                    self.Spawn<shapes::Circle>().Position({20, 12}).Radius(5).Paint(RED).Fill();
                });
        });

    scene.Spawn<shapes::Rect>().Position({190, 60}).Size({120, 80}).Paint(SKYBLUE).Fill();

    // okrąg zmienia kolor pod kursorem
    scene.Spawn<shapes::Circle>()
        .Position({100, 260})
        .Radius(50)
        .Paint(ORANGE)
        .OnMouseEnter([](shapes::Circle& self) { self.Paint(RED); })
        .OnMouseLeave([](shapes::Circle& self) { self.Paint(ORANGE); });

    // koło: kliknięcie przełącza wypełnienie
    scene.Spawn<shapes::Circle>()
        .Position({250, 260})
        .Radius(50)
        .Paint(ORANGE)
        .Fill()
        .OnClick([filled = true](shapes::Circle& self) mutable { self.Fill(filled = !filled); });

    scene.Spawn<shapes::Triangle>()
        .Vertices({600, 80}, {540, 200}, {680, 200})
        .Paint(DARKGREEN)
        .OnClick([](shapes::Triangle& self) { self.Fill(); });

    scene.Spawn<shapes::Triangle>().Vertices({740, 80}, {680, 200}, {800, 200}).Paint(LIME).Fill();

    scene.Spawn<shapes::Polyline>()
        .Vertices({{450, 80}, {520, 130}, {495, 210}, {405, 210}, {380, 130}})
        .Closed()
        .Paint(PURPLE)
        .OnMouseEnter([](shapes::Polyline& self) { self.Paint(MAGENTA); })
        .OnMouseLeave([](shapes::Polyline& self) { self.Paint(PURPLE); });

    // porównanie: algorytm przyrostowy (czerwony) i raylib (niebieski)
    scene.Spawn<shapes::Line>().From({100, 400}).To({400, 550}).Incremental().Paint(RED);
    scene.Spawn<shapes::Line>().From({500, 400}).To({800, 550}).Paint(BLUE);

    // odcinek: PPM przesuwa koniec, SPACJA przełącza algorytm
    scene.Spawn<shapes::Line>()
        .From({500, 300})
        .To({700, 300})
        .Incremental()
        .Paint(RED)
        .OnUpdate([](shapes::Line& self, float) {
            const Engine& engine = Engine::Get();
            if (engine.MouseDown(MOUSE_BUTTON_RIGHT)) self.To(engine.MousePosition());
        })
        .OnKeyPressed(KEY_SPACE, [incremental = true](shapes::Line& self) mutable {
            incremental = !incremental;
            self.Incremental(incremental).Paint(incremental ? RED : BLUE);
        });

    // wersje tablicowe: chmura punktów i "gwiazda" z niezależnych odcinków
    scene.Spawn<shapes::Points>()
        .Position({830, 80})
        .Positions([] {
            std::vector<Vector2> positions;
            for (int i = 0; i < 300; ++i)
                positions.push_back({static_cast<float>(::GetRandomValue(0, 150)),
                                     static_cast<float>(::GetRandomValue(0, 120))});
            return positions;
        }())
        .Paint(MAROON);

    scene.Spawn<shapes::Lines>()
        .Position({900, 270})
        .Vertices([] {
            std::vector<Vector2> vertices;
            for (int i = 0; i < 16; ++i) {
                const float angle = static_cast<float>(i) * 2.0f * PI / 16.0f;
                vertices.push_back({0, 0});
                vertices.push_back({45 * std::cos(angle), 45 * std::sin(angle)});
            }
            return vertices;
        }())
        .Incremental()
        .Paint(DARKPURPLE);

    // układ słoneczny: planeta krąży wokół słońca, księżyc (dziecko planety) wokół planety
    scene.Spawn<shapes::Group>().Position({880, 420}).Children([](shapes::Group& system) {
        system.Spawn<shapes::Circle>().Radius(18).Paint(ORANGE).Fill(); // słońce w środku grupy

        system.Spawn<shapes::Circle>()
            .Radius(10)
            .Paint(SKYBLUE)
            .Fill()
            .OnUpdate([angle = 0.0f](shapes::Circle& self, float dt) mutable {
                angle += dt;
                self.Position({50 * std::cos(angle), 50 * std::sin(angle)});
            })
            .Children([](shapes::Circle& planet) {
                planet.Spawn<shapes::Circle>().Radius(4).Paint(GRAY).Fill().OnUpdate(
                    [angle = 0.0f](shapes::Circle& self, float dt) mutable {
                        angle += 4 * dt;
                        self.Position({18 * std::cos(angle), 18 * std::sin(angle)});
                    });
            });
    });

    // ---------------- Point2D, LineSegment i łamane z nich (lab 2, zad. 4-6) ----------------
    // łamana otwarta i zamknięta ze zbioru punktów Point2D
    const std::vector<Point2D> zigzag{{340, 240}, {370, 270}, {400, 240}, {430, 270}, {460, 240}};
    const std::vector<Point2D> diamond{{400, 285}, {430, 305}, {400, 325}, {370, 305}};
    // łamana zamknięta ze zbioru odcinków - może się przecinać (Polygon() by jej nie narysował)
    const std::vector<LineSegment> bowTie{{{340, 340}, {460, 380}},
                                          {{460, 380}, {460, 340}},
                                          {{460, 340}, {340, 380}},
                                          {{340, 380}, {340, 340}}};
    // odcinki rysowane metodą LineSegment::Draw - te same nachylenia dwoma algorytmami
    const std::vector<LineSegment> library{{{830, 510}, {980, 545}}, {{830, 525}, {865, 600}}};
    const std::vector<LineSegment> incremental{{{845, 510}, {995, 545}}, {{845, 525}, {880, 600}}};

    scene.OnDraw([=](PrimitiveRenderer& renderer) {
        renderer.Paint(DARKGREEN).Incremental().Polyline(zigzag);
        renderer.Paint(DARKGREEN).Incremental(false).Polyline(diamond, true);
        renderer.Paint(MAROON).Incremental().Polyline(bowTie);

        renderer.Paint(BLUE);
        for (const LineSegment& segment : library)
            segment.Draw(renderer); // algorytm domyślny (raylib)
        renderer.Paint(RED);
        for (const LineSegment& segment : incremental)
            segment.Draw(renderer, true); // algorytm przyrostowy

        renderer.Paint(BLACK);
        for (const LineSegment& segment : library) {
            segment.From().Draw(renderer); // końce odcinków jako Point2D
            segment.To().Draw(renderer);
        }

        ::DrawText("lamane: Point2D / LineSegment", 335, 388, 10, GRAY);
        ::DrawText("Draw(): raylib / przyrostowy", 830, 608, 10, GRAY);
        ::DrawText("strzalki - prostokat, L - warstwa, PPM - odcinek, "
                   "SPACJA - algorytm, LPM - klik",
                   20, 630, 18, GRAY);
    });
}
