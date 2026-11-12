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

    // przycisk: podświetla się pod kursorem, kliknięcie przełącza scenę
    scene.Spawn<shapes::Rect>()
        .Position({400, 300})
        .Size({200, 60})
        .Paint(DARKBLUE)
        .OnMouseEnter([](shapes::Rect& self) { self.Filled(); })
        .OnMouseLeave([](shapes::Rect& self) { self.Filled(false); })
        .OnClick([](shapes::Rect&) { Engine::Get().SwitchScene("prymitywy"); });

    scene.OnDraw([](PrimitiveRenderer&) {
        ::DrawText("MENU", 455, 230, 40, DARKBLUE);
        ::DrawText("START", 470, 320, 24, GRAY);
        ::DrawText("Klawisze scen:", 240, 530, 20, DARKGRAY);
        ::DrawText("1 menu     2 prymitywy     3 figury     4 transformacje", 240, 560, 18, GRAY);
        ::DrawText("5 kontekst     6 trafienia     7 kamera     8 efekty", 240, 586, 18, GRAY);
        ::DrawText("9 formularz     0 sprite     - fizyka", 240, 612, 18, GRAY);
    });
}
