/**
 * @file PrimitivesScene.cpp
 * @brief Demonstracja figur, łamanych, warstw, dzieci i algorytmu odcinka (lab 2).
 * @details Scena pokazuje interakcję myszy i klawiatury, porównanie algorytmu
 * przyrostowego z funkcją biblioteki raylib oraz klasy Point2D i LineSegment.
 */

#include <cmath>
#include <string>
#include <vector>

#include <raycade>

/** @brief Konfiguruje interaktywną scenę prymitywów. */
SCENE("prymitywy") {
    // prostokąt sterowany strzałkami, L przenosi go nad / pod błękitny
    scene.Spawn<node::shapes::Rect>()
        .Position({40, 60})
        .Size({120, 80})
        .Paint(DARKBLUE)
        .Filled()
        .OnUpdate([&engine](node::shapes::Rect& self, float dt) {
            const Input& input = engine.Input();
            Vector2 position = self.Position();
            const float speed = 200 * dt;
            if (input.KeyDown(KEY_RIGHT)) position.x += speed;
            if (input.KeyDown(KEY_LEFT)) position.x -= speed;
            if (input.KeyDown(KEY_DOWN)) position.y += speed;
            if (input.KeyDown(KEY_UP)) position.y -= speed;
            self.Position(position);
        })
        .OnKeyPressed(
            KEY_L, [](node::shapes::Rect& self) { self.Layer(self.Layer() == 0 ? 1 : 0); }
        )
        // dzieci jadą razem z prostokątem - ich pozycje są względem niego
        .Children([](node::shapes::Rect& self) {
            self.Spawn<node::shapes::Circle>().Position({30, 30}).Radius(10).Paint(WHITE).Filled();
            self.Spawn<node::shapes::Rect>()
                .Position({70, 45})
                .Size({40, 25})
                .Paint(YELLOW)
                .Filled()
                .Children([](node::shapes::Rect& rect) { // wnuk: względem żółtego prostokąta
                    rect.Spawn<node::shapes::Circle>()
                        .Position({20, 12})
                        .Radius(5)
                        .Paint(RED)
                        .Filled();
                });
        });

    scene.Spawn<node::shapes::Rect>().Position({190, 60}).Size({120, 80}).Paint(SKYBLUE).Filled();

    // okrąg zmienia kolor pod kursorem
    scene.Spawn<node::shapes::Circle>()
        .Position({100, 260})
        .Radius(50)
        .Paint(ORANGE)
        .OnMouseEnter([](node::shapes::Circle& self) { self.Paint(RED); })
        .OnMouseLeave([](node::shapes::Circle& self) { self.Paint(ORANGE); });

    // koło: kliknięcie przełącza wypełnienie
    scene.Spawn<node::shapes::Circle>()
        .Position({250, 260})
        .Radius(50)
        .Paint(ORANGE)
        .Filled()
        .OnClick([filled = true](node::shapes::Circle& self) mutable {
            self.Filled(filled = !filled);
        });

    scene.Spawn<node::shapes::Triangle>()
        .Vertices({600, 80}, {540, 200}, {680, 200})
        .Paint(DARKGREEN)
        .OnClick([](node::shapes::Triangle& self) { self.Filled(); });

    scene.Spawn<node::shapes::Triangle>()
        .Vertices({740, 80}, {680, 200}, {800, 200})
        .Paint(LIME)
        .Filled();

    scene.Spawn<node::shapes::Polyline>()
        .Vertices({{450, 80}, {520, 130}, {495, 210}, {405, 210}, {380, 130}})
        .Closed()
        .Paint(PURPLE)
        .OnMouseEnter([](node::shapes::Polyline& self) { self.Paint(MAGENTA); })
        .OnMouseLeave([](node::shapes::Polyline& self) { self.Paint(PURPLE); });

    // porównanie: algorytm przyrostowy (czerwony) i raylib (niebieski)
    scene.Spawn<node::shapes::Line>()
        .From({100, 400})
        .To({400, 550})
        .Raster(Rasterizer::Custom)
        .Paint(RED);
    scene.Spawn<node::shapes::Line>().From({500, 400}).To({800, 550}).Paint(BLUE);

    // odcinek: PPM przesuwa koniec, SPACJA przełącza algorytm
    scene.Spawn<node::shapes::Line>()
        .From({500, 300})
        .To({700, 300})
        .Raster(Rasterizer::Custom)
        .Paint(RED)
        .OnUpdate([&engine](node::shapes::Line& self, float) {
            const Input& input = engine.Input();
            if (input.MouseDown(MOUSE_BUTTON_RIGHT)) self.To(input.MousePosition());
        })
        .OnKeyPressed(KEY_SPACE, [incremental = true](node::shapes::Line& self) mutable {
            incremental = !incremental;
            self.Raster(incremental ? Rasterizer::Custom : Rasterizer::Raylib)
                .Paint(incremental ? RED : BLUE);
        });

    // wersje tablicowe: chmura punktów i "gwiazda" z niezależnych odcinków
    scene.Spawn<node::shapes::Points>()
        .Position({830, 80})
        .Positions([] {
            std::vector<Point2D> positions;
            for (int i = 0; i < 300; ++i)
                positions.push_back(
                    {static_cast<float>(::GetRandomValue(0, 150)),
                     static_cast<float>(::GetRandomValue(0, 120))}
                );
            return positions;
        }())
        .Paint(MAROON);

    scene.Spawn<node::shapes::Lines>()
        .Position({900, 270})
        .Vertices([] {
            std::vector<Point2D> vertices;
            for (int i = 0; i < 16; ++i) {
                const float angle = static_cast<float>(i) * 2.0f * PI / 16.0f;
                vertices.push_back({0, 0});
                vertices.push_back({45 * std::cos(angle), 45 * std::sin(angle)});
            }
            return vertices;
        }())
        .Raster(Rasterizer::Custom)
        .Paint(DARKPURPLE);

    // układ słoneczny: planeta krąży wokół słońca, księżyc (dziecko planety) wokół planety
    scene.Spawn<node::Group>().Position({880, 420}).Children([](node::Group& system) {
        system.Spawn<node::shapes::Circle>()
            .Radius(18)
            .Paint(ORANGE)
            .Filled(); // słońce w środku grupy

        system.Spawn<node::shapes::Circle>()
            .Radius(10)
            .Paint(SKYBLUE)
            .Filled()
            .OnUpdate([angle = 0.0f](node::shapes::Circle& self, float dt) mutable {
                angle += dt;
                self.Position({50 * std::cos(angle), 50 * std::sin(angle)});
            })
            .Children([](node::shapes::Circle& planet) {
                planet.Spawn<node::shapes::Circle>().Radius(4).Paint(GRAY).Filled().OnUpdate(
                    [angle = 0.0f](node::shapes::Circle& self, float dt) mutable {
                        angle += 4 * dt;
                        self.Position({18 * std::cos(angle), 18 * std::sin(angle)});
                    }
                );
            });
    });

    // ---------------- Point2D, LineSegment i łamane z nich (lab 2, zad. 4-6) ----------------
    // łamana otwarta i zamknięta ze zbioru punktów Point2D
    const std::vector<Point2D> zigzag{{340, 240}, {370, 270}, {400, 240}, {430, 270}, {460, 240}};
    const std::vector<Point2D> diamond{{400, 285}, {430, 305}, {400, 325}, {370, 305}};
    // łamana zamknięta ze zbioru odcinków - może się przecinać (Polygon() by jej nie narysował)
    const std::vector<LineSegment> bowTie{
        {{340, 340}, {460, 380}},
        {{460, 380}, {460, 340}},
        {{460, 340}, {340, 380}},
        {{340, 380}, {340, 340}}
    };
    // odcinki rysowane metodą LineSegment::Draw - te same nachylenia dwoma algorytmami
    const std::vector<LineSegment> library{{{830, 510}, {980, 545}}, {{830, 525}, {865, 600}}};
    const std::vector<LineSegment> incremental{{{845, 510}, {995, 545}}, {{845, 525}, {880, 600}}};

    scene.OnDraw([=](PrimitiveRenderer& renderer) {
        renderer.Paint(DARKGREEN).Raster(Rasterizer::Custom).Polyline(zigzag);
        renderer.Paint(DARKGREEN).Raster(Rasterizer::Raylib).Polyline(diamond, true);
        renderer.Paint(MAROON).Raster(Rasterizer::Custom).Polyline(bowTie);

        renderer.Paint(BLUE);
        for (const LineSegment& segment : library)
            segment.Draw(renderer.Raster(Rasterizer::Raylib)); // algorytm raylib
        renderer.Paint(RED);
        for (const LineSegment& segment : incremental)
            segment.Draw(renderer.Raster(Rasterizer::Custom)); // algorytm przyrostowy

        renderer.Paint(BLACK);
        for (const LineSegment& segment : library) {
            segment.From().Draw(renderer); // końce odcinków jako Point2D
            segment.To().Draw(renderer);
        }
    });

    // ---------------- podpisy ----------------
    const auto label = [&scene](Vector2 position, std::string text, float size) {
        scene.Spawn<node::visual::Text>()
            .Content(std::move(text))
            .FontSize(size)
            .Paint(GRAY)
            .Position(position);
    };
    label({335, 388}, "łamane: Point2D / LineSegment", 10);
    label({830, 608}, "Draw(): raylib / przyrostowy", 10);
    label(
        {20, 630},
        "strzałki - prostokąt, L - warstwa, PPM - odcinek, SPACJA - algorytm, LPM - klik", 18
    );
}
