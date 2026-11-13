/**
 * @file MenuScene.cpp
 * @brief Scena startowa z animowaną piłką i przyciskiem START.
 * @details Przycisk reaguje na najechanie kursora i przełącza na scenę prymitywów.
 */

#include <cmath>

#include <raycade>

/** @brief Konfiguruje scenę menu po otwarciu okna. */
SCENE("menu") {
    scene.Spawn<shapes::Rect>().Position({300, 200}).Size({400, 300}).Paint(DARKBLUE);

    // piłka z własną logiką ruchu
    scene.Spawn<shapes::Circle>()
        .Position({500, 440})
        .Radius(20)
        .Paint(ORANGE)
        .Filled()
        .OnUpdate([time = 0.0f](shapes::Circle& self, float dt) mutable {
            time += dt;
            self.Position({500 + 150 * std::sin(time * 2), 440});
        });

    scene.Spawn<visual::Text>()
        .Content("MENU")
        .FontSize(40)
        .Paint(DARKBLUE)
        .Anchor({0.5f, 0})
        .Position({500, 230});

    // przycisk: podświetla się pod kursorem, kliknięcie przełącza scenę
    scene.Spawn<shapes::Rect>()
        .Position({400, 300})
        .Size({200, 60})
        .Paint(DARKBLUE)
        .OnMouseEnter([](shapes::Rect& self) { self.Filled(); })
        .OnMouseLeave([](shapes::Rect& self) { self.Filled(false); })
        .OnClick([&engine](shapes::Rect&) { engine.SwitchScene("prymitywy"); })
        .Children([](shapes::Rect& button) {
            // napis bez zdarzeń - kliknięcie w niego trafia w przycisk
            button.Spawn<visual::Text>()
                .Content("START")
                .FontSize(24)
                .Paint(GRAY)
                .Anchor({0.5f, 0.5f})
                .Position({100, 30});
        });

    scene.Spawn<visual::Text>()
        .Content("Klawisze scen:")
        .FontSize(20)
        .Paint(DARKGRAY)
        .Position({240, 530});
    scene.Spawn<visual::Text>()
        .Content(
            "1 menu     2 prymitywy     3 figury     4 transformacje\n"
            "5 kontekst     6 trafienia     7 kamera     8 efekty\n"
            "9 formularz     0 sprite     - fizyka     = dźwięk"
        )
        .FontSize(18)
        .Paint(GRAY)
        .LineHeight(1.45f)
        .Position({240, 560});
}
