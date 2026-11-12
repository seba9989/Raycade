/**
 * @file TransformScene.cpp
 * @brief Pokaz figur przed i po transformacji z laboratorium 4.
 */

#include <raycade>

/** @brief Tworzy trzy porównania transformacji. */
SCENE("transformacje") {
    // Warstwa 1 rysuje szare kontury stanu przed transformacją nad kolorowymi figurami.
    scene.Spawn<shapes::Rect>().Position({170, 150}).Size({170, 90}).Paint(GRAY).Layer(1);
    scene.Spawn<shapes::Rect>()
        .Position({170, 150})
        .Size({170, 90})
        .Paint(DARKBLUE)
        .Fill()
        .Rotate(PI / 6.0f, {255, 195})
        .OnClick([](shapes::Rect& self) { self.Paint(RED); });

    scene.Spawn<shapes::Circle>().Position({650, 200}).Radius(65).Paint(GRAY).Layer(1);
    scene.Spawn<shapes::Circle>()
        .Position({650, 200})
        .Radius(65)
        .Paint(SKYBLUE)
        .Fill()
        .Scale({1.5f, 0.6f}); // względem własnej pozycji (650, 200)

    scene.Spawn<Group>().Position({200, 440}).Layer(1).Children([](Group& self) {
        self.Spawn<shapes::Rect>().Position({-50, -30}).Size({100, 60}).Paint(GRAY);
        self.Spawn<shapes::Circle>().Position({80, 0}).Radius(25).Paint(GRAY);
    });

    scene.Spawn<Group>()
        .Position({200, 440})
        .Children([](Group& self) {
            self.Spawn<shapes::Rect>().Position({-50, -30}).Size({100, 60}).Paint(GOLD).Fill();
            self.Spawn<shapes::Circle>().Position({80, 0}).Radius(25).Paint(ORANGE).Fill();
        })
        .Rotate(PI / 5.0f); // wokół własnej pozycji (200, 440)

    // Podpisy i punkty obrotu/skalowania są rysowane już we współrzędnych ekranu.
    scene.OnDraw([](PrimitiveRenderer&) {
        ::DrawText("Szary kontur = PRZED     Kolor = PO", 30, 70, 20, DARKGRAY);
        ::DrawText("Obrot prostokata o 30 stopni (kliknij)", 80, 300, 18, DARKGRAY);
        ::DrawText("Skalowanie kola: X 1.5, Y 0.6", 520, 300, 18, DARKGRAY);
        ::DrawText("Obrot grupy razem z dziecmi", 80, 560, 18, DARKGRAY);
        ::DrawCircle(255, 195, 3, MAROON);
        ::DrawCircle(650, 200, 3, MAROON);
        ::DrawCircle(200, 440, 3, MAROON);
    });
}
