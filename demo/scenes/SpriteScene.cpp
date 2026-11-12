/**
 * @file SpriteScene.cpp
 * @brief Scena testowa visual::Sprite i SpriteSheet: animacje, tryby, sterowanie, współdzielenie.
 */
#include <raycade>

#include <algorithm>
#include <memory>
#include <string>

namespace {

constexpr int kFrame = 32;    /**< Rozmiar klatki arkusza. */
constexpr int kMargin = 1;    /**< Margines arkusza (lewy i górny). */
constexpr int kSpacing = 2;   /**< Odstęp między klatkami. */
constexpr float kZoom = 2.0f; /**< Powiększenie podglądu arkusza. */

/**
 * @brief Arkusz testowy w assets/: 8 x 3 klatek 32 x 32, margines 1 px, odstęp 2 px.
 * @details Każda klatka ma swój numer, więc widać, którą klatkę arkusza pokazuje sprite.
 * Margines i odstępy są magentowe - jeśli siatka byłaby źle policzona, magenta pojawi się
 * na krawędzi sprite'a. Biały "nos" po prawej i żółty pasek u góry pokazują FlippedX() i
 * FlippedY(). Plik można edytować w dowolnym programie graficznym i przeładować klawiszem L.
 */
constexpr const char* kFile = "sprite-test.png";

/**
 * @brief Opis arkusza w formacie eksportu Aseprite (json-array, --list-tags).
 * @details Klatki z czasami w milisekundach i tagi: stoi, biegnie, waha (pingpong), atak
 * (repeat 1 - raz) i cofa (reverse). Obraz wskazuje pole meta.image ("sprite-test.png").
 */
constexpr const char* kJson = "sprite-test.json";

/** @brief Nazwa trybu do opisu. */
const char* ModeName(const SpriteSheet& sheet, const std::string& clip) {
    const SpriteAnimation* found = sheet.FindAnimation(clip);
    if (!found) return "cały arkusz";
    switch (found->mode) {
    case Playback::Loop:
        return "Loop";
    case Playback::Once:
        return "Once";
    case Playback::PingPong:
        return "PingPong";
    }
    return "";
}

} // namespace

SCENE("sprite") {
    Engine& engine = Engine::Get();
    scene.ProvideContext<DefaultTypeface>(engine.Assets().Font("font.ttf"));
    const auto label =
        [&scene](Vector2 position, std::string text, float size = 15) -> visual::Text& {
        return scene.Spawn<visual::Text>()
            .Content(std::move(text))
            .FontSize(size)
            .Paint(DARKGRAY)
            .Position(position);
    };

    // ---------------- wspólny arkusz z animacjami (z pliku) ----------------
    // Assets wczytuje plik raz; brakujący plik daje szachownicę zastępczą i wpis w logu
    // Assets::Sheet: klatki, czasy i animacje z tagów z pliku JSON, obraz z meta.image przez
    // tę samą pamięć podręczną co Bitmap()
    const auto fromFile = engine.Assets().Sheet(kJson);
    const auto bitmap = fromFile->Bitmap();
    // arkusz z Assets jest tylko do odczytu - kopia (bitmapa zostaje wspólna), do której
    // dopisujemy to, czego plik Aseprite nie zapisuje: listę klatek z różnych wierszy i zdarzenia
    const auto sheet = std::make_shared<SpriteSheet>(*fromFile);
    sheet->Animation("lista", {0, 8, 16, 23, 15, 7}, 3)
        .Event("biegnie", 2, "krok")
        .Event("biegnie", 6, "krok")
        .Event("atak", 5, "trafienie");

    label({40, 24}, "Sprite: test", 26).Paint(BLACK);
    auto& file = label({40, 58}, "", 13);
    // linia stanu pliku; reloads - ile razy przeładowano klawiszem L
    const auto describe = [&file, sheet](const Bitmap& image, bool ok, int reloads) {
        std::string text = std::string{kJson} + " + .png: ";
        text += ok ? ::TextFormat(
                         "%dx%d, klatki %d, animacje %d", image.Width(), image.Height(),
                         sheet->FrameCount(), static_cast<int>(sheet->Animations().size())
                     )
                   : "BRAK - szachownica zastępcza";
        if (reloads > 0) text += ::TextFormat("   przeładowano (L): %d", reloads);
        file.Content(text).Paint(ok ? DARKGREEN : MAROON);
    };
    describe(*bitmap, bitmap->IsLoaded(), 0);

    // ---------------- podgląd arkusza z ramką bieżącej klatki ----------------
    const Vector2 preview{420, 70};
    label(
        {preview.x, preview.y - 22},
        "Arkusz z pliku JSON (Aseprite: klatki, czasy, tagi) - ramka = SheetFrame()"
    );
    auto& sheetView =
        scene.Spawn<visual::Sprite>().Sheet(bitmap).Position(preview).Scale({kZoom, kZoom});
    auto& marker = scene.Spawn<shapes::Rect>()
                       .Size({kFrame * kZoom, kFrame * kZoom})
                       .Paint(RED)
                       .Position(preview)
                       .Layer(1);
    // druga ramka 1 px w głąb - grubsza, lepiej widoczna na każdym kolorze klatki
    marker.Spawn<shapes::Rect>()
        .Size({kFrame * kZoom - 2, kFrame * kZoom - 2})
        .Paint(RED)
        .Position({1, 1});

    // ---------------- główny sprite ----------------
    auto& info = label({40, 270}, "", 15).LineHeight(1.35f);
    auto& events = label({420, 330}, "Zdarzenia (OnFrameEvent / OnFinish):", 14).LineHeight(1.35f);
    auto log = std::make_shared<std::vector<std::string>>();
    const auto addEvent = [&events, log](const std::string& line) {
        log->push_back(line);
        if (log->size() > 4) log->erase(log->begin());
        std::string text = "Zdarzenia (OnFrameEvent / OnFinish):";
        for (const std::string& entry : *log)
            text += "\n" + entry;
        events.Content(text);
    };

    auto& hero = scene.Spawn<visual::Sprite>()
                     .Sheet(sheet)
                     .Origin({kFrame / 2.0f, kFrame / 2.0f})
                     .Position({150, 160})
                     .Scale({4, 4})
                     .Play("stoi");
    hero.OnFinish([addEvent](visual::Sprite& self, const std::string& name) {
            addEvent("koniec \"" + name + "\" -> stoi");
            self.Play("stoi");
        })
        .OnFrameEvent([addEvent](visual::Sprite& self, const std::string& event) {
            addEvent(event + " (klatka " + std::to_string(self.Frame()) + ")");
        })
        // trafienie myszą liczy się w obróconej i przeskalowanej klatce (Contains + Origin)
        .OnClick([addEvent](visual::Sprite& self) {
            const bool tinted = !::ColorIsEqual(self.Paint(), WHITE);
            self.Paint(tinted ? WHITE : Color{255, 160, 160, 255});
            addEvent("klik w sprite");
        });

    const std::pair<int, const char*> clips[] = {{KEY_Q, "stoi"},  {KEY_W, "biegnie"},
                                                 {KEY_E, "waha"},  {KEY_R, "atak"},
                                                 {KEY_T, "lista"}, {KEY_C, "cofa"}};
    for (const auto& [key, name] : clips)
        hero.OnKeyPressed(key, [clip = std::string{name}](visual::Sprite& self) {
            self.Play(clip);
        });
    hero.OnKeyPressed(
            KEY_SPACE,
            [](visual::Sprite& self) {
                if (self.IsPlaying())
                    self.Pause();
                else
                    self.Play();
            }
    )
        .OnKeyPressed(KEY_S, [](visual::Sprite& self) { self.Stop(); })
        .OnKeyPressed(KEY_LEFT, [](visual::Sprite& self) { self.Pause().Frame(self.Frame() - 1); })
        .OnKeyPressed(KEY_RIGHT, [](visual::Sprite& self) { self.Pause().Frame(self.Frame() + 1); })
        .OnKeyPressed(
            KEY_UP, [](visual::Sprite& self) { self.Speed(std::min(8.0f, self.Speed() * 2)); }
        )
        .OnKeyPressed(
            KEY_DOWN, [](visual::Sprite& self) { self.Speed(std::max(0.125f, self.Speed() / 2)); }
        )
        .OnKeyPressed(KEY_X, [](visual::Sprite& self) { self.FlippedX(!self.IsFlippedX()); })
        .OnKeyPressed(KEY_Y, [](visual::Sprite& self) { self.FlippedY(!self.IsFlippedY()); })
        .OnKeyPressed(KEY_Z, [](visual::Sprite& self) { self.Rotate(PI / 12); })
        .OnKeyPressed(KEY_P, [](visual::Sprite& self) { self.Play("nie-ma-takiej"); }) // log raz
        // L: plik od nowa z dysku (z pominięciem Assets) i podmiana bitmapy we wspólnym arkuszu -
        // wszystkie sprite'y z tym arkuszem od razu pokazują nową wersję
        .OnKeyPressed(
            KEY_L,
            [sheet, &sheetView, describe, addEvent, reloads = 0](visual::Sprite&) mutable {
                auto fresh = Shared(Bitmap::Load(Engine::Get().Assets().Path(kFile)));
                if (!fresh) {
                    addEvent("L: nie można wczytać pliku");
                    return;
                }
                sheet->Bitmap(fresh);
                sheetView.Sheet(fresh);
                describe(*fresh, true, ++reloads);
                addEvent(::TextFormat("L: przeładowano (%dx%d)", fresh->Width(), fresh->Height()));
            }
        )
        .OnUpdate([&info, &marker, sheet, preview](visual::Sprite& self, float) {
            const Rectangle frame = sheet->Frame(self.SheetFrame());
            marker.Position({preview.x + frame.x * kZoom, preview.y + frame.y * kZoom});
            const Vector2 size = self.FrameSize();
            info.Content(
                "Animacja: " +
                (self.CurrentAnimation().empty() ? std::string{"(domyślna)"}
                                                 : self.CurrentAnimation()) +
                " [" + ModeName(*self.Sheet(), self.CurrentAnimation()) + "]\n" +
                "Klatka: " + std::to_string(self.Frame()) + "   w arkuszu: " +
                std::to_string(self.SheetFrame()) + " / " + std::to_string(self.SheetFrames()) +
                "\n" + "Gra: " + (self.IsPlaying() ? "tak" : "nie") +
                "   Speed: " + ::TextFormat("%.3g", self.Speed()) + "\n" +
                "FlippedX: " + (self.IsFlippedX() ? "tak" : "nie") +
                "   FlippedY: " + (self.IsFlippedY() ? "tak" : "nie") + "\n" +
                "FrameSize: " + std::to_string(static_cast<int>(size.x)) + "x" +
                std::to_string(static_cast<int>(size.y))
            );
        });

    label(
        {40, 432},
        "Q stoi  W biegnie  E waha (pingpong)  R atak (repeat 1 = Once, czasy klatek)  "
        "C cofa (reverse)  T lista klatek\n"
        "Spacja pauza/wznów  S stop  ← → klatka  ↑ ↓ szybkość  X/Y odbicie  Z obrót  "
        "P nieznana animacja (log)  L przeładuj PNG  LPM na postaci - kolor",
        14
    )
        .LineHeight(1.4f)
        .Paint(GRAY);

    // ---------------- wiele sprite'ów, jeden arkusz ----------------
    auto& shared = label({40, 478}, "");
    for (int i = 0; i < 16; ++i)
        scene.Spawn<visual::Sprite>()
            .Sheet(sheet)
            .Play("biegnie")
            .Frame(i % 8) // przesunięte fazy
            .Speed(0.5f + 0.1f * static_cast<float>(i % 6))
            .Position({40.0f + static_cast<float>(i) * 40, 502});
    // odczyt licznika co klatkę: wszystkie sprite'y trzymają ten sam std::shared_ptr
    shared.OnUpdate([sheet](visual::Text& self, float) {
        self.Content(
            "16 sprite'ów z jednym arkuszem (use_count arkusza: " +
            std::to_string(sheet.use_count()) + ") - różne fazy Frame() i Speed()"
        );
    });

    // ---------------- przypadki brzegowe ----------------
    const float row = 572;
    label({40, row}, "Animation() na wspólnym\narkuszu = prywatna kopia", 13);
    scene.Spawn<visual::Sprite>()
        .Sheet(sheet)
        .Animation("stoi", 16, 4, 2) // tylko ten sprite: "stoi" z pomarańczowego wiersza
        .Play("stoi")
        .Position({220, row});

    // obraz po nazwie z siatką: ten sam plik co meta.image w JSON - Assets daje tę samą bitmapę
    auto& byName = scene.Spawn<visual::Sprite>().Sheet(
        kFile, {kFrame, kFrame}, {kMargin, kMargin}, {kSpacing, kSpacing}
    );
    label(
        {300, row},
        std::string{"Sheet(\""} + kFile + "\", ...)\nta sama bitmapa (Assets): " +
            (byName.Sheet()->Bitmap() == bitmap ? "tak" : "NIE"),
        13
    );
    byName.Fps(4)
        .Play() // animacja domyślna: wszystkie 24 klatki
        .Position({490, row});

    label({570, row}, "Brakujący plik:\nSheet(\"brak.png\")", 13);
    scene.Spawn<visual::Sprite>().Sheet("brak.png", {16, 16}).Scale({2, 2}).Position({700, row});

    // JSON po nazwie: Sprite pobiera arkusz z Assets w kontekście - ten sam obiekt co fromFile
    auto& jsonByName = scene.Spawn<visual::Sprite>().Sheet(kJson);
    label(
        {760, row},
        std::string{"Sheet(\""} + kJson + "\")\nten sam arkusz (Assets): " +
            (jsonByName.Sheet() == fromFile ? "tak" : "NIE") + "\nobrót wokół Origin()",
        13
    );
    jsonByName.Play("waha")
        .Origin({kFrame / 2.0f, kFrame / 2.0f})
        .Position({960, row + 16})
        .OnUpdate([](visual::Sprite& self, float dt) { self.Rotate(dt * 1.5f); });
}
