/**
 * @file ShapesScene.cpp
 * @brief Demonstracja algorytmów okręgu, elipsy, wielokąta i wypełniania (lab 3).
 * @details Własne algorytmy okręgu i elipsy są porównywane z raylib: czerwone krzywe rysuje
 * algorytm własny, niebieskie - raylib. Płótno pozwala porównać flood fill i boundary fill.
 */

#include <array>
#include <string>
#include <vector>

#include <raycade>

/** @brief Konfiguruje scenę porównania algorytmów z raylib. */
SCENE("figury") {
    // ---------------- okrąg: 8-krotna symetria vs raylib ----------------
    scene.Spawn<node::shapes::Circle>()
        .Position({90, 110})
        .Radius(55)
        .Raster(Rasterizer::Custom)
        .Paint(RED);
    scene.Spawn<node::shapes::Circle>().Position({230, 110}).Radius(55).Paint(BLUE);
    // nałożone na siebie - widać różnice pojedynczych pikseli
    scene.Spawn<node::shapes::Circle>().Position({370, 110}).Radius(55).Paint(BLUE);
    scene.Spawn<node::shapes::Circle>()
        .Position({370, 110})
        .Radius(55)
        .Raster(Rasterizer::Custom)
        .Paint(RED)
        .Layer(1);

    scene.Spawn<node::shapes::Circle>()
        .Position({90, 245})
        .Radius(45)
        .Raster(Rasterizer::Custom)
        .Paint(RED)
        .Filled();
    scene.Spawn<node::shapes::Circle>().Position({230, 245}).Radius(45).Paint(BLUE).Filled();

    // ---------------- elipsa: 4-krotna symetria vs raylib ----------------
    scene.Spawn<node::shapes::Ellipse>()
        .Position({600, 110})
        .Radius({75, 40})
        .Raster(Rasterizer::Custom)
        .Paint(RED);
    scene.Spawn<node::shapes::Ellipse>().Position({760, 110}).Radius({75, 40}).Paint(BLUE);
    scene.Spawn<node::shapes::Ellipse>().Position({910, 110}).Radius({75, 40}).Paint(BLUE);
    scene.Spawn<node::shapes::Ellipse>()
        .Position({910, 110})
        .Radius({75, 40})
        .Raster(Rasterizer::Custom)
        .Paint(RED)
        .Layer(1);

    scene.Spawn<node::shapes::Ellipse>()
        .Position({600, 245})
        .Radius({40, 48})
        .Raster(Rasterizer::Custom)
        .Paint(RED)
        .Filled();
    scene.Spawn<node::shapes::Ellipse>().Position({760, 245}).Radius({40, 48}).Paint(BLUE).Filled();

    // ---------------- wielokąty ----------------
    const std::vector<Point2D> arrow{{0, 0}, {120, 50}, {60, 60}, {50, 120}};

    // kontur algorytmem przyrostowym; najechanie wypełnia go (scanline z parzystością)
    scene.Spawn<node::shapes::Polygon>()
        .Position({30, 360})
        .Vertices(arrow)
        .Raster(Rasterizer::Custom)
        .Paint(RED)
        .OnMouseEnter([](node::shapes::Polygon& self) { self.Filled(); })
        .OnMouseLeave([](node::shapes::Polygon& self) { self.Filled(false); });

    // wielokąt wklęsły wypełniony z kontrolą parzystości - wcięcie zostaje puste
    scene.Spawn<node::shapes::Polygon>()
        .Position({180, 360})
        .Vertices({{0, 0}, {130, 0}, {130, 120}, {65, 40}, {0, 120}})
        .Paint(RED)
        .Filled();

    // wielokąt zbudowany ze zbioru odcinków (LineSegment)
    scene.Spawn<node::shapes::Polygon>()
        .Position({340, 360})
        .Edges(
            {{{0, 50}, {60, 0}},
             {{60, 0}, {120, 50}},
             {{120, 50}, {120, 120}},
             {{120, 120}, {0, 120}},
             {{0, 120}, {0, 50}}}
        )
        .Raster(Rasterizer::Custom)
        .Paint(DARKGREEN);

    // łamana wiązana (krawędzie się przecinają) - metoda nic nie rysuje;
    // szare punkty pokazują tylko, gdzie leżą jej wierzchołki
    const std::vector<Point2D> bowTie{{0, 0}, {120, 90}, {120, 0}, {0, 90}};
    scene.Spawn<node::shapes::Polygon>()
        .Position({30, 520})
        .Vertices(bowTie)
        .Raster(Rasterizer::Custom)
        .Paint(RED);
    scene.Spawn<node::shapes::Points>()
        .Position({30, 520})
        .Positions({{0, 0}, {120, 90}, {120, 0}, {0, 90}})
        .Paint(GRAY)
        .Children([](node::shapes::Points& self) {
            for (const Vector2 corner : {Vector2{0, 0}, {120, 90}, {120, 0}, {0, 90}})
                self.Spawn<node::shapes::Circle>().Position(corner).Radius(3).Paint(GRAY);
        });

    // ---------------- wypełnianie przez spójność ----------------
    // Kontury rysowane własnymi algorytmami na płótnie (bitmapie w RAM).
    // LPM - flood fill, Shift + LPM - boundary fill (granica: czarny), R - od nowa.
    const auto paintContours = [](PrimitiveRenderer& r) {
        r.Paint(BLACK).Raster(Rasterizer::Custom);

        // prostokąt z kolorowymi paskami: flood fill zatrzyma się na paskach,
        // boundary fill zamaluje całe wnętrze aż do czarnej krawędzi
        r.Polygon(std::vector<Point2D>{{10, 10}, {190, 10}, {190, 130}, {10, 130}});
        r.Paint(GOLD).Filled().Rect({50, 11}, {20, 119}).Paint(SKYBLUE).Rect({120, 11}, {20, 119});

        // okrąg z czerwonym pierścieniem w środku
        r.Filled(false).Paint(BLACK).Circle({300, 70}, 60).Paint(RED).Circle({300, 70}, 30);

        // elipsa i wklęsły wielokąt
        r.Paint(BLACK).Ellipse({300, 205}, {95, 50});
        r.Polygon(std::vector<Point2D>{{20, 160}, {180, 160}, {180, 260}, {100, 200}, {20, 260}});
    };

    scene.Spawn<node::visual::Canvas>()
        .Position({560, 330})
        .Size({420, 280})
        .Background(WHITE)
        .Edit(paintContours)
        .OnClick([&engine, colorIndex = 0](node::visual::Canvas& self) mutable {
            // inne niż kolory konturów i pasków - boundary fill traktuje piksel w kolorze
            // wypełnienia jako już wypełniony i na nim się zatrzymuje
            static constexpr std::array<Color, 6> kColors{LIME,   PINK,  VIOLET,
                                                          ORANGE, BEIGE, MAGENTA};
            const Input& input = engine.Input();
            const Vector2 seed = self.ScreenToLocal(input.MousePosition());
            const bool boundary = input.KeyDown(KEY_LEFT_SHIFT) || input.KeyDown(KEY_RIGHT_SHIFT);
            const Color color = kColors[colorIndex++ % kColors.size()];

            self.Edit([&](PrimitiveRenderer& r) {
                if (boundary)
                    r.Paint(color).BoundaryFill(seed, BLACK);
                else
                    r.Paint(color).FloodFill(seed);
            });
        })
        .OnKeyPressed(KEY_R, [paintContours](node::visual::Canvas& self) {
            self.Clear().Edit(paintContours);
        });

    // ---------------- podpisy ----------------
    const auto label = [&scene](Vector2 position, std::string text, float size, Color color) {
        scene.Spawn<node::visual::Text>()
            .Content(std::move(text))
            .FontSize(size)
            .Paint(color)
            .Position(position);
    };
    label({20, 20}, "Okrąg (8-krotna symetria)", 18, DARKGRAY);
    label({520, 20}, "Elipsa (4-krotna symetria)", 18, DARKGRAY);
    label({50, 175}, "własny        raylib       nałożone", 14, GRAY);
    label({560, 175}, "własny          raylib          nałożone", 14, GRAY);
    label({20, 330}, "Wielokąty: kontur / scanline / z odcinków", 18, DARKGRAY);
    label({170, 560}, "łamana wiązana - nie rysuje się", 14, GRAY);
    label({560, 304}, "Wypełnianie przez spójność", 18, DARKGRAY);
    label(
        {20, 630},
        "LPM - flood fill, Shift+LPM - boundary fill, R - wyczyść płótno, "
        "czerwony - własny algorytm, niebieski - raylib",
        14, GRAY
    );
}
