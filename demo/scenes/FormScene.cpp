/**
 * @file FormScene.cpp
 * @brief Przykład ui::TextField: formularz z dwoma polami, walidacją i listą wpisów.
 */
#include <raycade>

#include <string>

SCENE("formularz") {
    Engine& engine = Engine::Get();

    // czcionka z polskimi literami dla całej sceny (pola i napisy biorą ją z kontekstu)
    scene.ProvideContext<DefaultTypeface>(engine.Assets().Font("font.ttf"));

    scene.Spawn<visual::Text>().Content("Zapisy na turniej").FontSize(28).Position({40, 30});

    // ---------------- etykiety i pola ----------------
    scene.Spawn<visual::Text>().Content("Imię").FontSize(16).Paint(GRAY).Position({40, 90});
    auto& name = scene.Spawn<ui::TextField>()
                     .Placeholder("np. Zażółć")
                     .MaxLength(24)
                     .Size({300, 40})
                     .FontSize(20)
                     .Position({40, 112});

    scene.Spawn<visual::Text>().Content("E-mail").FontSize(16).Paint(GRAY).Position({40, 170});
    auto& email = scene.Spawn<ui::TextField>()
                      .Placeholder("adres@example.com")
                      .MaxLength(40)
                      .Size({300, 40})
                      .FontSize(20)
                      .Position({40, 192});

    // komunikat pod polami i lista zapisanych osób
    auto& status = scene.Spawn<visual::Text>().FontSize(16).Position({40, 250});
    auto& list = scene.Spawn<visual::Text>()
                     .Content("Zapisani:")
                     .FontSize(18)
                     .LineHeight(1.4f)
                     .Position({400, 90});

    // licznik znaków aktualizowany przy każdej zmianie treści
    auto& counter = scene.Spawn<visual::Text>()
                        .Content("0/24")
                        .FontSize(14)
                        .Paint(GRAY)
                        .Anchor({1, 0}) // prawy górny róg napisu w pozycji
                        .Position({340, 90});
    name.OnChange([&counter](ui::TextField&, const std::string& text) {
        // MaxLength liczy znaki, nie bajty - "ż" to 2 bajty UTF-8
        counter.Content(std::to_string(::TextLength(text.c_str())) + "/24");
    });

    // wspólna akcja dla Entera w polach i przycisku
    const auto submit = [&name, &email, &status, &list, &counter] {
        const std::string who = name.Content();
        const std::string mail = email.Content();
        if (who.empty()) {
            status.Content("Podaj imię").Paint(MAROON);
            name.Focus();
            return;
        }
        if (mail.find('@') == std::string::npos) {
            status.Content("Nieprawidłowy e-mail").Paint(MAROON);
            email.Focus();
            return;
        }
        list.Content(list.Content() + "\n• " + who + " <" + mail + ">");
        status.Content("Zapisano: " + who).Paint(DARKGREEN);
        name.Content(""); // Content() nie wywołuje OnChange()
        email.Content("");
        counter.Content("0/24");
        name.Focus(); // od razu można wpisywać następną osobę
    };

    // Enter w imieniu przechodzi do e-maila, Enter w e-mailu zapisuje
    name.OnSubmit([&email](ui::TextField&, const std::string&) { email.Focus(); });
    email.OnSubmit([submit](ui::TextField&, const std::string&) { submit(); });

    // ---------------- przycisk ----------------
    auto& button = scene.Spawn<shapes::Rect>()
                       .Size({140, 40})
                       .Paint(SKYBLUE)
                       .Fill()
                       .Position({40, 290})
                       .OnClick([submit](shapes::Rect&) { submit(); });
    button.Spawn<visual::Text>()
        .Content("Zapisz")
        .FontSize(20)
        .Paint(WHITE)
        .Anchor({0.5f, 0.5f})
        .Position({70, 20});

    // bez fokusu na start: klawisze 1-9 dalej przełączają sceny, dopóki nie klikniesz pola
    // (w grze można tu wywołać name.Focus(), żeby pisać od razu)
}
