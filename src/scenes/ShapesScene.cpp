/**
 * @file ShapesScene.cpp
 * @brief Demonstracja algorytmów okręgu, elipsy, wielokąta i wypełniania (lab 3).
 * @details Własne algorytmy okręgu i elipsy są porównywane z raylib: czerwone krzywe rysuje
 * algorytm własny, niebieskie - raylib. Płótno pozwala porównać flood fill i boundary fill.
 */

#include <array>
#include <vector>

#include "lib/core/Engine.hpp"

/** @brief Konfiguruje scenę porównania algorytmów z raylib. */
SCENE("figury") {
    // ---------------- okrąg: 8-krotna symetria vs raylib ----------------
    scene.Spawn<shapes::Circle>().Position({90, 110}).Radius(55).Symmetric().Paint(RED);
    scene.Spawn<shapes::Circle>().Position({230, 110}).Radius(55).Paint(BLUE);
    // nałożone na siebie - widać różnice pojedynczych pikseli
    scene.Spawn<shapes::Circle>().Position({370, 110}).Radius(55).Paint(BLUE);
    scene.Spawn<shapes::Circle>().Position({370, 110}).Radius(55).Symmetric().Paint(RED).Layer(1);

    scene.Spawn<shapes::Circle>().Position({90, 245}).Radius(45).Symmetric().Paint(RED).Fill();
    scene.Spawn<shapes::Circle>().Position({230, 245}).Radius(45).Paint(BLUE).Fill();

    // ---------------- elipsa: 4-krotna symetria vs raylib ----------------
    scene.Spawn<shapes::Ellipse>().Position({600, 110}).Radius({75, 40}).Symmetric().Paint(RED);
    scene.Spawn<shapes::Ellipse>().Position({760, 110}).Radius({75, 40}).Paint(BLUE);
    scene.Spawn<shapes::Ellipse>().Position({910, 110}).Radius({75, 40}).Paint(BLUE);
    scene.Spawn<shapes::Ellipse>()
        .Position({910, 110})
        .Radius({75, 40})
        .Symmetric()
        .Paint(RED)
        .Layer(1);

    scene.Spawn<shapes::Ellipse>()
        .Position({600, 245})
        .Radius({40, 48})
        .Symmetric()
        .Paint(RED)
        .Fill();
    scene.Spawn<shapes::Ellipse>().Position({760, 245}).Radius({40, 48}).Paint(BLUE).Fill();

    // ---------------- wielokąty ----------------
    const std::vector<Point2D> arrow{{0, 0}, {120, 50}, {60, 60}, {50, 120}};

    // kontur algorytmem przyrostowym; najechanie wypełnia go (scanline z parzystością)
    scene.Spawn<shapes::Polygon>()
        .Position({30, 360})
        .Vertices(arrow)
        .Incremental()
        .Paint(RED)
        .OnMouseEnter([](shapes::Polygon& self) { self.Fill(); })
        .OnMouseLeave([](shapes::Polygon& self) { self.Fill(false); });

    // wielokąt wklęsły wypełniony z kontrolą parzystości - wcięcie zostaje puste
    scene.Spawn<shapes::Polygon>()
        .Position({180, 360})
        .Vertices({{0, 0}, {130, 0}, {130, 120}, {65, 40}, {0, 120}})
        .Paint(RED)
        .Fill();

    // wielokąt zbudowany ze zbioru odcinków (LineSegment)
    scene.Spawn<shapes::Polygon>()
        .Position({340, 360})
        .Edges({{{0, 50}, {60, 0}},
                {{60, 0}, {120, 50}},
                {{120, 50}, {120, 120}},
                {{120, 120}, {0, 120}},
                {{0, 120}, {0, 50}}})
        .Incremental()
        .Paint(DARKGREEN);

    // łamana wiązana (krawędzie się przecinają) - metoda nic nie rysuje;
    // szare punkty pokazują tylko, gdzie leżą jej wierzchołki
    const std::vector<Point2D> bowTie{{0, 0}, {120, 90}, {120, 0}, {0, 90}};
    scene.Spawn<shapes::Polygon>().Position({30, 520}).Vertices(bowTie).Incremental().Paint(RED);
    scene.Spawn<shapes::Points>()
        .Position({30, 520})
        .Positions({{0, 0}, {120, 90}, {120, 0}, {0, 90}})
        .Paint(GRAY)
        .Children([](shapes::Points& self) {
            for (const Vector2 corner : {Vector2{0, 0}, {120, 90}, {120, 0}, {0, 90}})
                self.Spawn<shapes::Circle>().Position(corner).Radius(3).Paint(GRAY);
        });

    // ---------------- wypełnianie przez spójność ----------------
    // Kontury rysowane własnymi algorytmami na płótnie (bitmapie w RAM).
    // LPM - flood fill, Shift + LPM - boundary fill (granica: czarny), R - od nowa.
    const auto paintContours = [](PrimitiveRenderer& r) {
        r.Paint(BLACK).Incremental().Symmetric();

        // prostokąt z kolorowymi paskami: flood fill zatrzyma się na paskach,
        // boundary fill zamaluje całe wnętrze aż do czarnej krawędzi
        r.Polygon(std::vector<Point2D>{{10, 10}, {190, 10}, {190, 130}, {10, 130}});
        r.Paint(GOLD).Fill().Rect({50, 11}, {20, 119}).Paint(SKYBLUE).Rect({120, 11}, {20, 119});

        // okrąg z czerwonym pierścieniem w środku
        r.Fill(false).Paint(BLACK).Circle({300, 70}, 60).Paint(RED).Circle({300, 70}, 30);

        // elipsa i wklęsły wielokąt
        r.Paint(BLACK).Ellipse({300, 205}, {95, 50});
        r.Polygon(std::vector<Point2D>{{20, 160}, {180, 160}, {180, 260}, {100, 200}, {20, 260}});
    };

    scene.Spawn<shapes::Canvas>()
        .Position({560, 330})
        .Size({420, 280})
        .Background(WHITE)
        .Content(paintContours)
        .OnClick([colorIndex = 0](shapes::Canvas& self) mutable {
            // inne niż kolory konturów i pasków - boundary fill traktuje piksel w kolorze
            // wypełnienia jako już wypełniony i na nim się zatrzymuje
            static constexpr std::array<Color, 6> kColors{LIME,   PINK,  VIOLET,
                                                          ORANGE, BEIGE, MAGENTA};
            const Engine& engine = Engine::Get();
            const Vector2 seed = self.ToLocal(engine.MousePosition());
            const bool boundary = engine.KeyDown(KEY_LEFT_SHIFT) || engine.KeyDown(KEY_RIGHT_SHIFT);
            const Color color = kColors[colorIndex++ % kColors.size()];

            self.Content([&](PrimitiveRenderer& r) {
                if (boundary)
                    r.Paint(color).BoundaryFill(seed, BLACK);
                else
                    r.Paint(color).FloodFill(seed);
            });
        })
        .OnKeyPressed(
            KEY_R, [paintContours](shapes::Canvas& self) { self.Clear().Content(paintContours); });

    // ---------------- podpisy ----------------
    scene.OnDraw([](PrimitiveRenderer&) {
        ::DrawText("Okrag (8-krotna symetria)", 20, 20, 18, DARKGRAY);
        ::DrawText("Elipsa (4-krotna symetria)", 520, 20, 18, DARKGRAY);
        ::DrawText("wlasny        raylib       nalozone", 50, 175, 14, GRAY);
        ::DrawText("wlasny          raylib          nalozone", 560, 175, 14, GRAY);
        ::DrawText("Wielokaty: kontur / scanline / z odcinkow", 20, 330, 18, DARKGRAY);
        ::DrawText("lamana wiazana - nie rysuje sie", 170, 560, 14, GRAY);
        ::DrawText("Wypelnianie przez spojnosc", 560, 304, 18, DARKGRAY);
        ::DrawText("LPM - flood fill, Shift+LPM - boundary fill, R - wyczysc plotno, "
                   "czerwony - wlasny algorytm, niebieski - raylib",
                   20, 630, 14, GRAY);
    });
}
