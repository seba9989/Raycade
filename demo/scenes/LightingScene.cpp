/**
 * @file LightingScene.cpp
 * @brief Dynamiczne oświetlenie 2D liczone w shaderze fragmentów.
 * @details Cały świat sceny rysuje się do jednej grupy node::visual::RenderGroup, a jej shader
 * oświetla gotowy obraz piksel po pikselu:
 * - światła punktowe i stożkowe (pozycja, kolor, zasięg, natężenie, kąt i kierunek stożka),
 *   przekazywane co klatkę tablicami uniformów (ShaderProgram::Uniform() ze std::span),
 * - zanik z odległością, mieszanie addytywne kolorów i poświata wokół źródeł,
 * - cienie: dla każdego piksela i światła shader sprawdza, czy odcinek piksel-światło przecina
 *   przeszkodę (prostokąt - metoda płyt, koło - najbliższy punkt odcinka). Tryb miękki traktuje
 *   źródło jak tarczę i próbkuje jej rozciągłość w poprzek promienia, co daje półcień; próbki
 *   pośrednie liczą się tylko na krawędziach cieni,
 * - mapowanie tonów (1 - e^-x), żeby nakładające się światła nie przepalały obrazu.
 *
 * Wszystko jest dynamiczne: gracz (WASD) nosi latarkę celowaną myszą, reflektor w lewym pokoju
 * się obraca, pochodnie migoczą, trzy kolorowe światła krążą wokół kolumny, blok jeździ w górę
 * i w dół, a skrzynie przeciąga się LPM. PPM stawia nowe światło, C usuwa postawione, F włącza
 * latarkę, T zmienia tryb cieni (brak / twarde / miękkie), O wstrzymuje krążenie, L wyłącza
 * shader (porównanie z obrazem bez oświetlenia), a kółko myszy zmienia światło otoczenia.
 */

#include <raycade>

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <memory>
#include <span>
#include <vector>

namespace {

/** @brief Rozmiar oświetlanego świata (grupy z shaderem) w pikselach gry. */
constexpr Vector2 kWorld{1000, 620};

/** @name Pojemność tablic uniformów - muszą zgadzać się z MAX_* w kLightingShader */
/** @{ */
constexpr std::size_t kMaxLights = 16;
constexpr std::size_t kMaxBoxes = 16;
constexpr std::size_t kMaxCircles = 8;
/** @} */

/**
 * @brief Shader oświetlenia grupy.
 * @details Tekstura RenderGroup jest w OpenGL odwrócona w pionie, a grupa rysuje ją
 * z odwróconym źródłem - dlatego górnej krawędzi grupy odpowiada v = 1. Położenie piksela
 * w układzie grupy to więc (u, 1 - v) * size; w tym samym układzie są światła i przeszkody.
 */
constexpr const char* kLightingShader = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;

#define MAX_LIGHTS 16
#define MAX_BOXES 16
#define MAX_CIRCLES 8
#define SOFT_SAMPLES 5

uniform vec2 size;          // rozmiar grupy w pikselach
uniform vec3 ambient;       // światło otoczenia
uniform float exposure;     // naświetlenie przed mapowaniem tonów
uniform int shadowMode;     // 0 - bez cieni, 1 - twarde, 2 - miękkie
uniform float lightSize;    // promień źródła w trybie miękkim

uniform int lightCount;
uniform vec4 lightShape[MAX_LIGHTS]; // x, y, zasięg, natężenie
uniform vec4 lightColor[MAX_LIGHTS]; // r, g, b, cos połowy kąta stożka (< -1 - we wszystkie strony)
uniform vec2 lightDir[MAX_LIGHTS];   // kierunek stożka (wektor jednostkowy)

uniform int boxCount;
uniform vec4 boxes[MAX_BOXES];       // x, y, szerokość, wysokość
uniform int circleCount;
uniform vec3 circles[MAX_CIRCLES];   // x, y, promień

out vec4 finalColor;

bool InBox(vec2 p, vec4 box) {
    return p.x >= box.x && p.y >= box.y && p.x <= box.x + box.z && p.y <= box.y + box.w;
}

bool InCircle(vec2 p, vec3 circle) {
    vec2 d = p - circle.xy;
    return dot(d, d) <= circle.z * circle.z;
}

// metoda płyt: przedział parametru t, w którym odcinek a + (b - a) t jest w prostokącie
bool SegmentHitsBox(vec2 a, vec2 b, vec4 box) {
    vec2 d = b - a;
    vec2 safe = vec2(abs(d.x) < 1e-4 ? 1e-4 : d.x, abs(d.y) < 1e-4 ? 1e-4 : d.y);
    vec2 t0 = (box.xy - a) / safe;
    vec2 t1 = (box.xy + box.zw - a) / safe;
    vec2 near = min(t0, t1);
    vec2 far = max(t0, t1);
    float enter = max(near.x, near.y);
    float leave = min(far.x, far.y);
    return enter <= leave && leave >= 0.0 && enter <= 1.0;
}

// najbliższy środkowi koła punkt odcinka leży bliżej niż promień
bool SegmentHitsCircle(vec2 a, vec2 b, vec3 circle) {
    vec2 d = b - a;
    float t = clamp(dot(circle.xy - a, d) / max(dot(d, d), 1e-4), 0.0, 1.0);
    vec2 diff = a + d * t - circle.xy;
    return dot(diff, diff) < circle.z * circle.z;
}

// przeszkoda zawierająca piksel albo światło nie rzuca cienia na tę parę: ściany są oświetlone
// same, a światło postawione na skrzyni (lub latarka gracza) nie gaśnie
bool Occluded(vec2 p, vec2 light) {
    for (int i = 0; i < MAX_BOXES; ++i) {
        if (i >= boxCount) break;
        vec4 box = boxes[i];
        if (InBox(p, box) || InBox(light, box)) continue;
        if (SegmentHitsBox(p, light, box)) return true;
    }
    for (int i = 0; i < MAX_CIRCLES; ++i) {
        if (i >= circleCount) break;
        vec3 circle = circles[i];
        if (InCircle(p, circle) || InCircle(light, circle)) continue;
        if (SegmentHitsCircle(p, light, circle)) return true;
    }
    return false;
}

float Hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float Visibility(vec2 p, vec2 light) {
    if (shadowMode == 0) return 1.0;
    float center = Occluded(p, light) ? 0.0 : 1.0;
    if (shadowMode == 1) return center;

    // miękkie cienie: źródło to tarcza o promieniu lightSize. W 2D o półcieniu decyduje tylko
    // jej rozciągłość w poprzek promienia, więc próbkuje się odcinek prostopadły do kierunku
    // piksel-światło. Gdy środek i oba końce dają to samo, piksel leży w pełnym świetle albo
    // w pełnym cieniu (3 testy); dopiero na krawędzi cienia dochodzą próbki pośrednie.
    vec2 toLight = light - p;
    vec2 side = vec2(-toLight.y, toLight.x) * (lightSize / max(length(toLight), 1e-3));
    float left = Occluded(p, light - side) ? 0.0 : 1.0;
    float right = Occluded(p, light + side) ? 0.0 : 1.0;
    if (left == center && right == center) return center;

    // przesunięcie próbek losowe w każdym pikselu - zamiast kilku ostrych kopii cienia
    // powstaje gładki półcień z drobnym szumem
    float jitter = Hash(p);
    float lit = center + left + right;
    for (int k = 0; k < SOFT_SAMPLES; ++k) {
        float t = (float(k) + jitter) / float(SOFT_SAMPLES) * 2.0 - 1.0;
        lit += Occluded(p, light + side * t) ? 0.0 : 1.0;
    }
    return lit / float(SOFT_SAMPLES + 3);
}

void main() {
    vec4 albedo = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    vec2 p = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y) * size;

    vec3 light = ambient;
    vec3 glow = vec3(0.0);
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (i >= lightCount) break;
        vec4 shape = lightShape[i];
        vec4 color = lightColor[i];
        vec2 toPixel = p - shape.xy;
        float dist = length(toPixel);

        // poświata źródła - widać, skąd świeci, nawet na jasnym tle
        glow += color.rgb * shape.w * 0.9 * exp(-dist * dist / 70.0);

        if (dist >= shape.z) continue;
        float falloff = 1.0 - dist / shape.z;
        float intensity = shape.w * falloff * falloff;
        if (color.w > -1.0) {
            float facing = dot(toPixel / max(dist, 1e-3), lightDir[i]);
            intensity *= smoothstep(color.w, mix(color.w, 1.0, 0.3), facing);
        }
        if (intensity < 0.002) continue;
        light += color.rgb * intensity * Visibility(p, shape.xy);
    }

    vec3 hdr = albedo.rgb * light * exposure + glow;
    finalColor = vec4(vec3(1.0) - exp(-hdr), albedo.a);
})";

/** @brief Źródło światła sceny. */
struct Light {
    Vector2 position{0, 0};  /**< Położenie w układzie świata. */
    Color color = WHITE;     /**< Barwa. */
    float range = 300;       /**< Zasięg w pikselach (dalej światło nie dociera). */
    float power = 1;         /**< Natężenie bazowe. */
    float cone = -2;         /**< Cosinus połowy kąta stożka; poniżej -1 - światło punktowe. */
    Vector2 direction{1, 0}; /**< Kierunek stożka (jednostkowy). */
    float flicker = 0;       /**< Siła migotania (0 - stałe), przesunięcie fazy w seed. */
    float seed = 0;          /**< Faza migotania. */
    bool enabled = true;     /**< Czy świeci. */
};

/** @name Indeksy stałych świateł w LightingState::lights */
/** @{ */
constexpr std::size_t kFlashlight = 0;
constexpr std::size_t kBeacon = 1;
constexpr std::size_t kFirstTorch = 2;
constexpr std::size_t kTorchCount = 4;
constexpr std::size_t kFirstOrbit = kFirstTorch + kTorchCount;
constexpr std::size_t kOrbitCount = 3;
constexpr std::size_t kFixedLights = kFirstOrbit + kOrbitCount; /**< Dalej - postawione PPM. */
/** @} */

/** @brief Środek kolumny, wokół której krążą kolorowe światła. */
constexpr Vector2 kOrbitCenter{652, 310};
/** @brief Położenie reflektora w lewym pokoju. */
constexpr Vector2 kBeaconPosition{160, 310};

/** @brief Stan sceny współdzielony przez wywołania zwrotne. */
struct LightingState {
    std::vector<Light> lights;                  /**< Światła; pierwsze kFixedLights są stałe. */
    std::vector<Rectangle> walls;               /**< Ściany zewnętrzne (tylko kolizje gracza). */
    std::vector<node::shapes::Rect*> boxes;     /**< Prostokątne przeszkody rzucające cień. */
    std::vector<node::shapes::Circle*> columns; /**< Okrągłe przeszkody rzucające cień. */
    node::shapes::Circle* player = nullptr;     /**< Gracz (też rzuca cień). */
    std::shared_ptr<ShaderProgram> shader;      /**< Shader oświetlenia (może być pusty). */
    int shadowMode = 2;                         /**< 0 - brak, 1 - twarde, 2 - miękkie. */
    float ambient = 0.06f;                      /**< Poziom światła otoczenia. */
    float orbitAngle = 0;                       /**< Kąt kolorowych świateł. */
    bool orbiting = true;                       /**< Czy kolorowe światła krążą. */
    bool lit = true;                            /**< Czy shader jest włączony. */
};

/** @brief Migotanie płomienia: suma kilku sinusów o niewspółmiernych częstotliwościach. */
float Flicker(float time, float seed) {
    return 0.12f * std::sin(time * 11.0f + seed) + 0.08f * std::sin(time * 23.7f + seed * 2.1f) +
           0.05f * std::sin(time * 5.3f + seed * 0.7f);
}

/** @brief Czy koło gracza w punkcie koliduje z którąś ścianą lub przeszkodą. */
bool Blocked(const LightingState& state, Vector2 position, float radius) {
    for (const Rectangle& wall : state.walls)
        if (::CheckCollisionCircleRec(position, radius, wall)) return true;
    for (const node::shapes::Rect* box : state.boxes) {
        const Vector2 p = box->Position();
        const Vector2 s = box->Size();
        if (::CheckCollisionCircleRec(position, radius, {p.x, p.y, s.x, s.y})) return true;
    }
    for (const node::shapes::Circle* column : state.columns)
        if (column != state.player &&
            ::CheckCollisionCircles(position, radius, column->Position(), column->Radius()))
            return true;
    return false;
}

/** @brief Wysyła do shadera światła i przeszkody z bieżącej klatki. */
void UploadUniforms(const LightingState& state, float time) {
    ShaderProgram& shader = *state.shader;

    std::array<Vector4, kMaxLights> shapes{};
    std::array<Vector4, kMaxLights> colors{};
    std::array<Vector2, kMaxLights> directions{};
    std::size_t lights = 0;
    for (const Light& light : state.lights) {
        if (!light.enabled || lights == kMaxLights) continue;
        const float power = light.power * (1.0f + light.flicker * Flicker(time, light.seed));
        const Vector4 color = ::ColorNormalize(light.color);
        shapes[lights] = {light.position.x, light.position.y, light.range, power};
        colors[lights] = {color.x, color.y, color.z, light.cone};
        directions[lights] = light.direction;
        ++lights;
    }

    std::array<Vector4, kMaxBoxes> boxes{};
    std::size_t boxCount = 0;
    for (const node::shapes::Rect* box : state.boxes) {
        if (boxCount == kMaxBoxes) break;
        const Vector2 p = box->Position();
        const Vector2 s = box->Size();
        boxes[boxCount++] = {p.x, p.y, s.x, s.y};
    }

    std::array<Vector3, kMaxCircles> circles{};
    std::size_t circleCount = 0;
    for (const node::shapes::Circle* column : state.columns) {
        if (circleCount == kMaxCircles) break;
        const Vector2 p = column->Position();
        circles[circleCount++] = {p.x, p.y, column->Radius()};
    }

    const float a = state.ambient;
    shader.Uniform("size", kWorld)
        .Uniform("ambient", Vector3{a * 0.6f, a * 0.7f, a * 1.0f})
        .Uniform("exposure", 2.4f)
        .Uniform("shadowMode", state.shadowMode)
        .Uniform("lightSize", 9.0f)
        .Uniform("lightCount", static_cast<int>(lights))
        .Uniform("lightShape", std::span<const Vector4>{shapes.data(), lights})
        .Uniform("lightColor", std::span<const Vector4>{colors.data(), lights})
        .Uniform("lightDir", std::span<const Vector2>{directions.data(), lights})
        .Uniform("boxCount", static_cast<int>(boxCount))
        .Uniform("boxes", std::span<const Vector4>{boxes.data(), boxCount})
        .Uniform("circleCount", static_cast<int>(circleCount))
        .Uniform("circles", std::span<const Vector3>{circles.data(), circleCount});
}

/** @brief Kolor z lekkim losowym rozjaśnieniem albo przyciemnieniem (płytki, deski). */
Color Vary(Color base, int amount) {
    const int delta = Engine::Get().RandomInt(-amount, amount);
    const auto channel = [delta](unsigned char c) {
        return static_cast<unsigned char>(std::clamp(c + delta, 0, 255));
    };
    return {channel(base.r), channel(base.g), channel(base.b), base.a};
}

/** @brief Podłoga: deski w lewym pokoju, kamienne płytki w prawym (dzieci grupy świata). */
void SpawnFloor(node::visual::RenderGroup& world) {
    for (float y = 20; y < 600; y += 20)
        world.Spawn<node::shapes::Rect>()
            .Position({20, y})
            .Size({280, 19})
            .Paint(Vary(Color{112, 78, 48, 255}, 10))
            .Filled();
    for (float y = 20; y < 600; y += 40)
        for (float x = 324; x < 980; x += 41)
            world.Spawn<node::shapes::Rect>()
                .Position({x, y})
                .Size({std::min(40.0f, 980.0f - x), std::min(39.0f, 600.0f - y)})
                .Paint(Vary(Color{92, 90, 96, 255}, 8))
                .Filled();
}

/** @brief Pochodnia na ścianie: uchwyt i płomień; światło dopisuje się do stanu. */
void SpawnTorch(node::visual::RenderGroup& world, LightingState& state, Vector2 flame, float seed) {
    const bool top = flame.y < kWorld.y / 2;
    world.Spawn<node::shapes::Rect>()
        .Position({flame.x - 4, top ? 6.0f : flame.y + 2})
        .Size({8, top ? flame.y - 8 : 612 - flame.y})
        .Paint(Color{60, 40, 25, 255})
        .Filled();
    world.Spawn<node::shapes::Circle>()
        .Position(flame)
        .Radius(4)
        .Paint(Color{255, 200, 90, 255})
        .Filled();
    state.lights.push_back(
        Light{
            .position = flame,
            .color = Color{255, 150, 60, 255},
            .range = 230,
            .power = 1.1f,
            .flicker = 1.0f,
            .seed = seed,
        }
    );
}

/** @brief Skrzynia przeciągana LPM - przesuwa się razem ze swoim cieniem. */
void SpawnCrate(node::visual::RenderGroup& world, LightingState& state, Vector2 position) {
    auto& crate =
        world.Spawn<node::shapes::Rect>()
            .Position(position)
            .Size({44, 44})
            .Paint(Color{160, 110, 60, 255})
            .Filled()
            .OnDrag([](node::shapes::Rect& self, Vector2 delta) {
                self.Translate(delta);
                const Vector2 p = self.Position();
                self.Position({std::clamp(p.x, 20.0f, 936.0f), std::clamp(p.y, 20.0f, 556.0f)});
            })
            .Children([](node::shapes::Rect& self) {
                self.Spawn<node::shapes::Rect>().Position({5, 5}).Size({34, 34}).Paint(
                    Color{100, 65, 30, 255}
                );
                self.Spawn<node::shapes::Line>().From({5, 5}).To({39, 39}).Paint(
                    Color{100, 65, 30, 255}
                );
            });
    state.boxes.push_back(&crate);
}

} // namespace

/** @brief Konfiguruje scenę oświetlenia. */
SCENE("swiatlo") {
    auto state = std::make_shared<LightingState>();
    state->shader = Shared(ShaderProgram::FromCode("", kLightingShader));
    if (!state->shader) engine.Log(LogLevel::Warning, "shader oswietlenia sie nie skompilowal");

    auto& world = scene.Spawn<node::visual::RenderGroup>()
                      .Size(kWorld)
                      .Background(Color{22, 20, 24, 255})
                      .Shader(state->shader);

    SpawnFloor(world);

    // ---------------- ściany ----------------
    const Color wallColor{70, 68, 80, 255};
    const auto wall = [&](Rectangle r, bool occluder) {
        auto& rect = world.Spawn<node::shapes::Rect>()
                         .Position({r.x, r.y})
                         .Size({r.width, r.height})
                         .Paint(wallColor)
                         .Filled();
        // ściany zewnętrzne nie zasłaniają niczego wewnątrz - shader ich nie sprawdza
        if (occluder)
            state->boxes.push_back(&rect);
        else
            state->walls.push_back(r);
    };
    wall({0, 0, 1000, 20}, false);
    wall({0, 600, 1000, 20}, false);
    wall({0, 0, 20, 620}, false);
    wall({980, 0, 20, 620}, false);
    wall({300, 20, 24, 220}, true); // ścianka działowa z przejściem 240-380
    wall({300, 380, 24, 220}, true);

    // ---------------- przeszkody ----------------
    const auto column = [&](Vector2 center, float radius) {
        auto& circle = world.Spawn<node::shapes::Circle>()
                           .Position(center)
                           .Radius(radius)
                           .Paint(Color{170, 160, 145, 255})
                           .Filled()
                           .Children([radius](node::shapes::Circle& self) {
                               self.Spawn<node::shapes::Circle>()
                                   .Radius(radius - 5)
                                   .Paint(Color{120, 112, 100, 255});
                           });
        state->columns.push_back(&circle);
    };
    for (const Vector2 center :
         std::initializer_list<Vector2>{{90, 140}, {230, 140}, {90, 480}, {230, 480}})
        column(center, 22);
    column(kOrbitCenter, 34);

    for (const Vector2 corner :
         std::initializer_list<Vector2>{{450, 120}, {810, 120}, {450, 456}, {810, 456}})
        state->boxes.push_back(&world.Spawn<node::shapes::Rect>()
                                    .Position(corner)
                                    .Size({44, 44})
                                    .Paint(Color{135, 125, 115, 255})
                                    .Filled());

    // blok jeżdżący w górę i w dół - jego cień przesuwa się po płytkach
    state->boxes.push_back(&world.Spawn<node::shapes::Rect>()
                                .Position({915, 60})
                                .Size({36, 80})
                                .Paint(Color{110, 125, 140, 255})
                                .Filled()
                                .OnUpdate([&engine](node::shapes::Rect& self, float) {
                                    const float t = static_cast<float>(engine.Time());
                                    self.Position({915, 290 + 220 * std::sin(t * 0.7f)});
                                }));

    SpawnCrate(world, *state, {40, 286});
    SpawnCrate(world, *state, {196, 530});

    // ---------------- światła ----------------
    // latarka gracza - stożek celowany myszą
    state->lights.push_back(
        Light{
            .color = Color{255, 240, 210, 255},
            .range = 460,
            .power = 1.7f,
            .cone = std::cos(24.0f * DEG2RAD),
        }
    );
    // reflektor - obracający się wąski stożek (przez przejście sięga do drugiego pokoju)
    world.Spawn<node::shapes::Circle>()
        .Position(kBeaconPosition)
        .Radius(9)
        .Paint(Color{60, 60, 60, 255})
        .Filled();
    state->lights.push_back(
        Light{
            .position = kBeaconPosition,
            .color = Color{255, 210, 120, 255},
            .range = 620,
            .power = 1.6f,
            .cone = std::cos(14.0f * DEG2RAD),
        }
    );
    float seed = 0;
    for (const Vector2 flame :
         std::initializer_list<Vector2>{{565, 30}, {745, 30}, {565, 590}, {745, 590}})
        SpawnTorch(world, *state, flame, seed += 1.7f);
    for (const Color color :
         std::initializer_list<Color>{{255, 40, 40, 255}, {40, 255, 60, 255}, {50, 90, 255, 255}})
        state->lights.push_back(Light{.color = color, .range = 330, .power = 1.3f});

    // ---------------- gracz ----------------
    state->player =
        &world.Spawn<node::shapes::Circle>()
             .Position({160, 560})
             .Radius(13)
             .Paint(Color{70, 150, 220, 255})
             .Filled()
             .Children([](node::shapes::Circle& self) {
                 self.Spawn<node::shapes::Circle>().Radius(13).Paint(Color{20, 50, 90, 255});
             });
    state->columns.push_back(state->player); // gracz rzuca cień od innych świateł

    // ---------------- opis ----------------
    auto& status =
        scene.Spawn<node::visual::Text>().FontSize(15).Paint(DARKGRAY).Position({20, kWorld.y + 4});
    scene.Spawn<node::visual::Text>()
        .Content(
            "WASD ruch · mysz celuje latarką · F latarka · T cienie · O krążenie · L shader · "
            "PPM nowe światło · C usuń · kółko otoczenie · LPM skrzynie"
        )
        .FontSize(14)
        .Paint(GRAY)
        .Position({20, kWorld.y + 24});

    // ---------------- logika ----------------
    scene.OnUpdate([&engine, &world, &status, state](float dt) {
        const Input& input = engine.Input();
        const float time = static_cast<float>(engine.Time());
        auto& lights = state->lights;

        // gracz: ruch z kolizjami osobno w osiach (ślizga się wzdłuż ścian)
        node::shapes::Circle& player = *state->player;
        Vector2 move{
            static_cast<float>(input.KeyDown(KEY_D) - input.KeyDown(KEY_A)),
            static_cast<float>(input.KeyDown(KEY_S) - input.KeyDown(KEY_W))
        };
        if (const float length = std::hypot(move.x, move.y); length > 0) {
            const float step = 190.0f * dt / length;
            Vector2 p = player.Position();
            if (!Blocked(*state, {p.x + move.x * step, p.y}, player.Radius())) p.x += move.x * step;
            if (!Blocked(*state, {p.x, p.y + move.y * step}, player.Radius())) p.y += move.y * step;
            player.Position(p);
        }

        // latarka z gracza w stronę kursora
        Light& flashlight = lights[kFlashlight];
        flashlight.position = player.Position();
        const Vector2 mouse = input.MousePosition();
        const Vector2 aim{mouse.x - flashlight.position.x, mouse.y - flashlight.position.y};
        if (const float length = std::hypot(aim.x, aim.y); length > 1)
            flashlight.direction = {aim.x / length, aim.y / length};

        lights[kBeacon].direction = {std::cos(time * 1.2f), std::sin(time * 1.2f)};

        if (state->orbiting) state->orbitAngle += dt * 0.6f;
        for (std::size_t i = 0; i < kOrbitCount; ++i) {
            const float angle = state->orbitAngle + static_cast<float>(i) * 2.0f * PI / 3.0f;
            lights[kFirstOrbit + i].position = {
                kOrbitCenter.x + 150 * std::cos(angle), kOrbitCenter.y + 150 * std::sin(angle)
            };
        }

        // przełączniki
        if (input.KeyPressed(KEY_F)) flashlight.enabled = !flashlight.enabled;
        if (input.KeyPressed(KEY_T)) state->shadowMode = (state->shadowMode + 1) % 3;
        if (input.KeyPressed(KEY_O)) state->orbiting = !state->orbiting;
        if (input.KeyPressed(KEY_L) && state->shader) {
            state->lit = !state->lit;
            world.Shader(state->lit ? state->shader : nullptr);
        }
        if (input.KeyPressed(KEY_C)) lights.resize(kFixedLights);
        state->ambient = std::clamp(state->ambient + input.MouseWheel() * 0.02f, 0.0f, 0.6f);

        const bool inWorld = ::CheckCollisionPointRec(mouse, {0, 0, kWorld.x, kWorld.y});
        if (input.MousePressed(MOUSE_BUTTON_RIGHT) && inWorld && lights.size() < kMaxLights)
            lights.push_back(
                Light{
                    .position = mouse,
                    .color = ::ColorFromHSV(engine.RandomFloat(0, 360), 0.65f, 1.0f),
                    .range = 260,
                    .power = 1.1f,
                }
            );

        if (state->shader) UploadUniforms(*state, time);

        constexpr const char* kShadowNames[] = {"brak", "twarde", "miękkie"};
        const auto active = std::ranges::count_if(lights, [](const Light& l) { return l.enabled; });
        status.Content(
            "Światła: {}/{}   cienie: {}   otoczenie: {:.0f}%   shader: {}   FPS: {}", active,
            kMaxLights, kShadowNames[state->shadowMode], state->ambient * 100.0f,
            !state->shader ? "błąd kompilacji"
            : state->lit   ? "wł."
                           : "wył.",
            engine.CurrentFps()
        );
    });
}
