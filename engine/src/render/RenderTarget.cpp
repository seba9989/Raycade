/**
 * @file RenderTarget.cpp
 * @brief Render do tekstury ze stosem celów i przywracaniem macierzy.
 * @details raylib (5.5) ma tylko jeden poziom BeginTextureMode(): EndTextureMode() zawsze wraca
 * na ekran, a oba zerują macierz widoku (kamerę) i projekcję. Dodatkowo rlgl trzyma
 * przekształcenia z rlPushMatrix() w osobnej macierzy "transform" nakładanej na CPU, której
 * BeginTextureMode() nie rusza - bez izolacji dzieci obiektu rysowałyby się w teksturze
 * przesunięte o jego pozycję w świecie. Ten plik uzupełnia oba braki.
 */

#include <raycade/render/RenderTarget.hpp>

#include <raycade/render/ClipStack.hpp>

#include <rlgl.h>

#include <utility>
#include <vector>

namespace {

/** @brief Stan zapamiętany przy Begin(), przywracany przy End(). */
struct Frame {
    const RenderTarget* target; /**< Cel otwarty tym Begin(). */
    Matrix projection;          /**< Projekcja sprzed Begin(). */
    Matrix modelview;           /**< Macierz widoku (kamera, skala ekranu) sprzed Begin(). */
};

/** @brief Stos otwartych celów; pusty oznacza rysowanie na ekran. */
std::vector<Frame>& Stack() {
    static std::vector<Frame> stack;
    return stack;
}

/**
 * @brief Ustawia macierz bieżącego trybu na podaną (rlgl nie ma rlLoadMatrix).
 * @details Tablica w kolejności m0..m15, jak MatrixToFloat() z raymath - bez dołączania
 * raymath.h, który przy raylib z FetchContent dawałby ostrzeżenia kompilatora.
 */
void Load(const Matrix& m) {
    const float values[16] = {m.m0, m.m1, m.m2,  m.m3,  m.m4,  m.m5,  m.m6,  m.m7,
                              m.m8, m.m9, m.m10, m.m11, m.m12, m.m13, m.m14, m.m15};
    ::rlLoadIdentity();
    ::rlMultMatrixf(values);
}

} // namespace

RenderTarget::RenderTarget(int width, int height) : target_(::LoadRenderTexture(width, height)) {
    ::SetTextureFilter(target_.texture, TEXTURE_FILTER_POINT);
}

RenderTarget::~RenderTarget() {
    Release();
}

RenderTarget::RenderTarget(RenderTarget&& other) noexcept
    : target_(std::exchange(other.target_, {})) {
}

RenderTarget& RenderTarget::operator=(RenderTarget&& other) noexcept {
    if (this != &other) {
        Release();
        target_ = std::exchange(other.target_, {});
    }
    return *this;
}

void RenderTarget::Release() {
    // teksturę można zwolnić tylko przy żywym kontekście OpenGL (przed CloseWindow)
    if (target_.id != 0 && ::IsWindowReady()) ::UnloadRenderTexture(target_);
    target_ = {};
}

RenderTarget& RenderTarget::Smooth(bool value) {
    if (IsValid())
        ::SetTextureFilter(target_.texture, value ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
    return *this;
}

bool RenderTarget::IsActive() const {
    return !Stack().empty() && Stack().back().target == this;
}

int RenderTarget::Depth() {
    return static_cast<int>(Stack().size());
}

/**
 * @details Kolejność ma znaczenie:
 * 1. zapamiętanie projekcji i macierzy widoku (BeginTextureMode() je wyzeruje),
 * 2. BeginTextureMode() - opróżnia bufor wierzchołków do poprzedniego celu, podpina teksturę,
 *    ustawia projekcję na jej rozmiar i zeruje macierz widoku,
 * 3. ClipStack::BeginTarget() - wyłącza przycięcia z zewnątrz (scissor) na czas rysowania
 *    do tekstury,
 * 4. rlPushMatrix() + rlLoadIdentity() - odkłada przekształcenie obiektów (transform) na stos
 *    rlgl i zaczyna od jednostkowego, żeby rysunek w teksturze nie dziedziczył pozycji obiektu.
 */
void RenderTarget::Begin() const {
    if (!IsValid()) return;
    Stack().push_back({this, ::rlGetMatrixProjection(), ::rlGetMatrixModelview()});
    ::BeginTextureMode(target_);
    // przycięcia z zewnątrz nie dotyczą rysunku w teksturze (scissor raylib zostałby włączony)
    ClipStack::BeginTarget(Width(), Height());
    ::rlPushMatrix();
    ::rlLoadIdentity();
}

/**
 * @details Odwrotność Begin():
 * 1. powrót do poprzedniego celu: BeginTextureMode() poprzedniej tekstury albo
 *    EndTextureMode() dla ekranu - oba najpierw opróżniają bufor do tej tekstury,
 * 2. przywrócenie projekcji i macierzy widoku sprzed Begin(),
 * 3. rlPushMatrix() + 2x rlPopMatrix(): rlPopMatrix() zapisuje do macierzy bieżącego trybu,
 *    a krok 2 zostawił tryb macierzy widoku. Pierwsza para przełącza rlgl z powrotem na
 *    macierz przekształceń bez zmiany stosu, drugie rlPopMatrix() przywraca przekształcenie
 *    odłożone w Begin() (a na dnie stosu wraca do zwykłego trybu bez przekształceń),
 * 4. ClipStack::EndTarget() - przywrócenie przycięcia poprzedniego celu.
 */
void RenderTarget::End() const {
    if (!IsValid() || !IsActive()) return;
    const Frame frame = Stack().back();
    Stack().pop_back();

    if (Stack().empty())
        ::EndTextureMode();
    else
        ::BeginTextureMode(Stack().back().target->target_);

    ::rlMatrixMode(RL_PROJECTION);
    Load(frame.projection);
    ::rlMatrixMode(RL_MODELVIEW);
    Load(frame.modelview);

    ::rlPushMatrix();
    ::rlPopMatrix();
    ::rlPopMatrix();

    // po powrocie do poprzedniego celu - przycięcie liczone jest w jego pikselach
    ClipStack::EndTarget();
}

const RenderTarget& RenderTarget::Clear(Color color) const {
    if (!IsValid()) return *this;
    if (IsActive()) {
        ::ClearBackground(color);
        return *this;
    }
    Begin();
    ::ClearBackground(color);
    End();
    return *this;
}

void RenderTarget::Draw(Vector2 position, Color tint) const {
    Draw(
        Rectangle{
            position.x, position.y, static_cast<float>(Width()), static_cast<float>(Height())
        },
        tint
    );
}

void RenderTarget::Draw(Rectangle destination, Color tint) const {
    if (!IsValid()) return;
    // tekstury renderowane są w OpenGL odwrócone w pionie - stąd ujemna wysokość źródła
    const Rectangle source{0, 0, static_cast<float>(Width()), -static_cast<float>(Height())};
    ::DrawTexturePro(target_.texture, source, destination, {0, 0}, 0.0f, tint);
}
