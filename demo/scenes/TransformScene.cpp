/**
 * @file TransformScene.cpp
 * @brief Pokaz figur przed i po transformacji z laboratorium 4.
 */

#include <raycade>

#include <string>

/** @brief Tworzy trzy porównania transformacji. */
SCENE("transformacje") {
    // Warstwa 1 rysuje szare kontury stanu przed transformacją nad kolorowymi figurami.
    scene.Spawn<node::shapes::Rect>().Position({170, 150}).Size({170, 90}).Paint(GRAY).DrawLayer(1);
    scene.Spawn<node::shapes::Rect>()
        .Position({170, 150})
        .Size({170, 90})
        .Paint(DARKBLUE)
        .Filled()
        .Rotate(PI / 6.0f, {255, 195})
        .OnClick([](node::shapes::Rect& self) { self.Paint(RED); });

    scene.Spawn<node::shapes::Circle>().Position({650, 200}).Radius(65).Paint(GRAY).DrawLayer(1);
    scene.Spawn<node::shapes::Circle>()
        .Position({650, 200})
        .Radius(65)
        .Paint(SKYBLUE)
        .Filled()
        .ScaleBy({1.5f, 0.6f}); // względem własnej pozycji (650, 200)

    scene.Spawn<node::Group>().Position({200, 440}).DrawLayer(1).Children([](node::Group& self) {
        self.Spawn<node::shapes::Rect>().Position({-50, -30}).Size({100, 60}).Paint(GRAY);
        self.Spawn<node::shapes::Circle>().Position({80, 0}).Radius(25).Paint(GRAY);
    });

    scene.Spawn<node::Group>()
        .Position({200, 440})
        .Children([](node::Group& self) {
            self.Spawn<node::shapes::Rect>()
                .Position({-50, -30})
                .Size({100, 60})
                .Paint(GOLD)
                .Filled();
            self.Spawn<node::shapes::Circle>().Position({80, 0}).Radius(25).Paint(ORANGE).Filled();
        })
        .Rotate(PI / 5.0f); // wokół własnej pozycji (200, 440)

    // Punkty obrotu i skalowania nad figurami.
    for (const Vector2 pivot : {Vector2{255, 195}, Vector2{650, 200}, Vector2{200, 440}})
        scene.Spawn<node::shapes::Circle>()
            .Position(pivot)
            .Radius(3)
            .Paint(MAROON)
            .Filled()
            .DrawLayer(2);

    const auto label = [&scene](Vector2 position, std::string text, float size, Color color) {
        scene.Spawn<node::visual::Text>()
            .Content(std::move(text))
            .FontSize(size)
            .Paint(color)
            .Position(position);
    };
    label({30, 70}, "Szary kontur = PRZED     Kolor = PO", 20, DARKGRAY);
    label({80, 300}, "Obrót prostokąta o 30 stopni (kliknij)", 18, DARKGRAY);
    label({520, 300}, "Skalowanie koła: X 1.5, Y 0.6", 18, DARKGRAY);
    label({80, 560}, "Obrót grupy razem z dziećmi", 18, DARKGRAY);
}
