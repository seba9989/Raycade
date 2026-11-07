/**
 * @file Engine.cpp
 * @brief Inicjalizacja okna, pętla gry, obsługa scen i diagnostyka.
 * @details Settery okna przed Init() tylko zapisują ustawienia, a po Init() zmieniają
 * okno na bieżąco.
 */

#include <raycade/Engine.hpp>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <stdexcept>

namespace {

/**
 * @brief Przekazuje komunikaty raylib do Engine::Log() (konsola i plik logu).
 * @details raylib sam filtruje poziom (SetTraceLogLevel) przed wywołaniem tej funkcji.
 */
void RaylibLog(int level, const char* text, va_list args) {
    va_list copy;
    va_copy(copy, args);
    const int size = std::vsnprintf(nullptr, 0, text, copy);
    va_end(copy);
    std::string message(size > 0 ? static_cast<std::size_t>(size) : 0, '\0');
    if (size > 0) std::vsnprintf(message.data(), message.size() + 1, text, args);

    LogLevel engineLevel = LogLevel::Info;
    switch (level) {
    case LOG_TRACE:
    case LOG_DEBUG:
        engineLevel = LogLevel::Debug;
        break;
    case LOG_WARNING:
        engineLevel = LogLevel::Warning;
        break;
    case LOG_ERROR:
    case LOG_FATAL:
        engineLevel = LogLevel::Error;
        break;
    default:
        break;
    }
    Engine::Get().Log(engineLevel, "[raylib] " + message);
}

/** @brief Znacznik poziomu na początku wpisu w logu (bez polskich znaków - konsola Windows). */
const char* LevelName(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return "[DEBUG] ";
    case LogLevel::Info:
        return "[INFO] ";
    case LogLevel::Warning:
        return "[UWAGA] ";
    case LogLevel::Error:
        return "[BLAD] ";
    }
    return "";
}

} // namespace

Contextable* ContextRoot() {
    return &Engine::Get();
}

Engine& Engine::Title(std::string title) {
    title_ = std::move(title);
    if (initialized_) ::SetWindowTitle(title_.c_str());
    return *this;
}

Engine& Engine::Window(int width, int height) {
    width_ = width;
    height_ = height;
    fullscreen_ = false;
    if (initialized_) {
        if (::IsWindowFullscreen()) ::ToggleFullscreen();
        ::SetWindowSize(width_, height_);
    }
    return *this;
}

Engine& Engine::Fullscreen(bool value) {
    fullscreen_ = value;
    if (!initialized_ || ::IsWindowFullscreen() == value) return *this;
    if (value) {
        // ToggleFullscreen() ustawia monitor na tryb o rozmiarze okna (np. 1000x700);
        // najpierw powiększamy okno do natywnej rozdzielczości monitora
        const int monitor = ::GetCurrentMonitor();
        ::SetWindowSize(::GetMonitorWidth(monitor), ::GetMonitorHeight(monitor));
        ::ToggleFullscreen();
    } else {
        ::ToggleFullscreen();
        ::SetWindowSize(width_, height_);
    }
    return *this;
}

Engine& Engine::Resolution(int width, int height) {
    viewWidth_ = width;
    viewHeight_ = height;
    if (initialized_ && scaling_ != Scaling::None) CreateTarget();
    return *this;
}

Engine& Engine::ScaleMode(Scaling mode) {
    scaling_ = mode;
    if (initialized_) {
        if (mode == Scaling::None)
            ReleaseTarget();
        else if (!target_.IsValid())
            CreateTarget();
        else
            UpdateScaling(); // nowy tryb działa od razu, a nie od następnej klatki
    }
    return *this;
}

Engine& Engine::PostShader(std::shared_ptr<ShaderProgram> shader) {
    postShader_ = std::move(shader);
    if (postShader_ && scaling_ == Scaling::None)
        Log(LogLevel::Warning, "PostShader() dziala tylko przy ScaleMode() innym niz None");
    return *this;
}

Engine& Engine::SmoothScaling(bool value) {
    smoothScaling_ = value;
    target_.Smooth(value);
    return *this;
}

Vector2 Engine::ViewSize() const {
    if (scaling_ != Scaling::None && viewWidth_ > 0)
        return {static_cast<float>(viewWidth_), static_cast<float>(viewHeight_)};
    return {static_cast<float>(ScreenWidth()), static_cast<float>(ScreenHeight())};
}

void Engine::CreateTarget() {
    ReleaseTarget();
    if (viewWidth_ <= 0 || viewHeight_ <= 0) {
        viewWidth_ = width_;
        viewHeight_ = height_;
    }
    target_ = RenderTarget{viewWidth_, viewHeight_};
    target_.Smooth(smoothScaling_);
    UpdateScaling();
}

void Engine::ReleaseTarget() {
    target_ = RenderTarget{};
    if (initialized_) input_.Transform({0, 0}, {1, 1});
}

/**
 * @details Prostokąt docelowy zależy od trybu:
 * - Stretch: całe okno,
 * - Fit: skala min(okno / gra) z zachowaniem proporcji, wyśrodkowana,
 * - PixelPerfect: jak Fit, ale skala zaokrąglona w dół do całkowitej (co najmniej 1).
 *
 * Mysz: raylib liczy (pozycja + przesunięcie) * skala, więc przesunięcie to minus lewy górny
 * róg obrazu, a skala to rozdzielczość gry / rozmiar obrazu w oknie.
 */
void Engine::UpdateScaling() {
    if (!target_.IsValid()) return;
    const float screenWidth = static_cast<float>(::GetScreenWidth());
    const float screenHeight = static_cast<float>(::GetScreenHeight());
    const float viewWidth = static_cast<float>(viewWidth_);
    const float viewHeight = static_cast<float>(viewHeight_);

    if (scaling_ == Scaling::Stretch) {
        destination_ = {0, 0, screenWidth, screenHeight};
    } else {
        float scale = std::min(screenWidth / viewWidth, screenHeight / viewHeight);
        // całkowita skala tylko, gdy obraz mieści się w oknie co najmniej 1:1; w mniejszym
        // oknie zachowujemy się jak Fit, zamiast ucinać obraz
        if (scaling_ == Scaling::PixelPerfect && scale >= 1.0f) scale = std::floor(scale);
        const float width = viewWidth * scale;
        const float height = viewHeight * scale;
        destination_ = {std::floor((screenWidth - width) / 2),
                        std::floor((screenHeight - height) / 2), width, height};
    }
    input_.Transform({-destination_.x, -destination_.y},
                     {viewWidth / destination_.width, viewHeight / destination_.height});
}

Engine& Engine::VSync(bool value) {
    return WindowFlag(FLAG_VSYNC_HINT, value, "VSync", false);
}

bool Engine::IsVSync() const {
    return (windowFlags_ & FLAG_VSYNC_HINT) != 0;
}

Engine& Engine::Fps(int fps) {
    fps_ = fps;
    if (initialized_) ::SetTargetFPS(fps_);
    return *this;
}

Engine& Engine::Init() {
    if (initialized_) return *this;
    if (!logFile_.empty()) log_.open(logFile_);
    // przed InitWindow, żeby do logu trafiły też komunikaty o tworzeniu okna
    ::SetTraceLogLevel(raylibLogLevel_);
    ::SetTraceLogCallback(RaylibLog);

    // flagi okna muszą być ustawione przed InitWindow
    unsigned int flags = windowFlags_;
    if (fullscreen_) flags |= FLAG_FULLSCREEN_MODE;
    ::SetConfigFlags(flags);

    if (fullscreen_)
        ::InitWindow(0, 0, title_.c_str());
    else
        ::InitWindow(width_, height_, title_.c_str());

    if (!::IsWindowReady()) {
        Log(LogLevel::Error, "nie udalo sie utworzyc okna");
        return *this;
    }

    ::SetTargetFPS(fps_);
    ::SetExitKey(exitKey_);
    // monitor przed pozycją - pozycja okna jest liczona na całym pulpicie
    if (monitor_ && *monitor_ >= 0 && *monitor_ < ::GetMonitorCount())
        ::SetWindowMonitor(*monitor_);
    else if (monitor_)
        Log(LogLevel::Warning, "nie ma monitora " + std::to_string(*monitor_) + " - pominieto");
    if (windowPosition_)
        ::SetWindowPosition(static_cast<int>(windowPosition_->x),
                            static_cast<int>(windowPosition_->y));
    if (opacity_) ::SetWindowOpacity(*opacity_);
    if (minSize_) ::SetWindowMinSize(minSize_->first, minSize_->second);
    if (maxSize_) ::SetWindowMaxSize(maxSize_->first, maxSize_->second);
    initialized_ = true;
    ApplyIcon();
    if (scaling_ != Scaling::None) CreateTarget();
    Log("Silnik zainicjalizowany (podwojne buforowanie, V-Sync: " +
        std::string(IsVSync() ? "tak" : "nie") + ")");
    return *this;
}

int Engine::Run() {
    if (!Init().IsReady()) return 1;
    LoadStartScene();
    double previous = ::GetTime();
    loadSeconds_ = 0.0;
    frameCount_ = 0;

    quit_ = false;
    while (!quit_ && !::WindowShouldClose()) {
        // Własny pomiar zamiast GetFrameTime(): raylib wlicza czas wczytywania sceny do klatki
        // dopiero z opóźnieniem. Odejmujemy go, żeby obiekty nie "skoczyły" o duże dt.
        const double now = ::GetTime();
        float dt = static_cast<float>(std::max(0.0, now - previous - loadSeconds_));
        // długa przerwa (debugger, przeciąganie okna) nie może przesunąć obiektów o kilka sekund
        if (maxDeltaTime_ > 0.0f) dt = std::min(dt, maxDeltaTime_);
        frameTime_ = dt;
        previous = now;
        loadSeconds_ = 0.0;
        const auto found = scenes_.find(active_);
        ::Scene* scene = found != scenes_.end() ? &found->second : nullptr;

        // rozmiar okna mógł się zmienić (pełny ekran, zmiana rozmiaru) - przed odczytem myszy
        if (target_.IsValid()) UpdateScaling();
        // pole tekstowe z fokusem (z poprzedniej klatki) zabiera klawiaturę reszcie gry,
        // także globalnemu OnUpdate - np. cyfry nie przełączają wtedy scen
        const bool captured = scene && scene->IsKeyboardCaptured();
        // Escape w polu tekstowym ma zdjąć fokus, a nie zamknąć okno (raylib sprawdza klawisz
        // wyjścia sam, niezależnie od przejęcia klawiatury)
        if (captured != input_.IsKeyboardCaptured()) ::SetExitKey(captured ? KEY_NULL : exitKey_);
        input_.captured_ = captured;

        PollSystemEvents();
        if (fixedStep_ > 0.0f) {
            fixedAccumulator_ += dt;
            int steps = 0;
            while (fixedAccumulator_ >= fixedStep_ && steps < maxFixedSteps_) {
                if (fixedUpdate_) fixedUpdate_(fixedStep_);
                if (scene) scene->FixedUpdate(fixedStep_);
                fixedAccumulator_ -= fixedStep_;
                ++steps;
            }
            // zaległość ponad limit kroków odrzucamy - inaczej rosłaby z każdą klatką
            if (steps == maxFixedSteps_)
                fixedAccumulator_ = std::min(fixedAccumulator_, fixedStep_);
        }
        if (update_) update_(dt);
        if (scene) scene->Update(dt, input_);

        // dopiero teraz: OnUpdate albo scena mogły w tej klatce włączyć lub wyłączyć skalowanie
        const bool scaled = target_.IsValid();
        // przy skalowaniu gra rysuje do tekstury w swojej rozdzielczości, a nie na ekran;
        // RenderTarget, bo tekstury obiektów (shapes::Surface) muszą wracać do niej, nie na ekran
        if (scaled)
            target_.Begin();
        else
            ::BeginDrawing();
        ::ClearBackground(background_);
        if (scene) scene->Draw(primitives_);
        if (draw_) draw_(primitives_.Reset().TargetScreen());
        if (scaled) {
            target_.End();
            ::BeginDrawing();
            ::ClearBackground(letterbox_);
            const bool post = postShader_ && postShader_->IsValid();
            if (post) ::BeginShaderMode(postShader_->Raw());
            target_.Draw(destination_);
            if (post) ::EndShaderMode();
        }
        ::EndDrawing();
        input_.UpdateActionState(); // stan osi pada do ActionPressed/Released w następnej klatce

        // zmiany scen między klatkami, poza pętlą po obiektach
        ApplySceneChanges();
        ++frameCount_;
    }

    Shutdown();
    return 0;
}

void Engine::Shutdown() {
    if (!initialized_) return;
    Log("Zamykanie silnika");
    // sceny przed oknem: obiekty z bitmapami zwalniają tekstury, co wymaga kontekstu OpenGL
    scenes_.clear();
    active_.clear();
    pending_.clear();
    unload_.clear();
    reload_.clear();
    ClearContext(); // wartości globalne też mogą trzymać tekstury
    if (recording_) {
        ::StopAutomationEventRecording();
        ::UnloadAutomationEventList(recordList_);
        recording_ = false;
        Log(LogLevel::Warning,
            "nagrywanie przerwane przy zamykaniu - nagranie nie zostalo zapisane");
    }
    if (replaying_) {
        ::UnloadAutomationEventList(replayList_);
        replaying_ = false;
    }
    postShader_.reset(); // zasób GPU - przed zamknięciem okna
    ReleaseTarget();
    ::CloseWindow();
    // komunikaty raylib po zamknięciu znów idą prosto na konsolę (silnik może już nie istnieć)
    ::SetTraceLogCallback(nullptr);
    initialized_ = false;
}

Scene& Engine::CreateScene(const std::string& name) {
    auto [it, created] = scenes_.try_emplace(name);
    if (!created) Log(LogLevel::Warning, "scena \"" + name + "\" juz istnieje");
    if (active_.empty()) active_ = name;
    return it->second;
}

Engine& Engine::CreateScene(const std::string& name, const std::function<void(::Scene&)>& setup) {
    setup(CreateScene(name));
    return *this;
}

Engine& Engine::RegisterScene(std::string name, std::function<void(::Scene&)> setup) {
    // przed main() plik logu nie jest jeszcze otwarty - komunikat trafi tylko na stderr
    if (registered_.contains(name)) {
        Log(LogLevel::Warning, "scena \"" + name + "\" zarejestrowana ponownie - pominieto");
        return *this;
    }
    registered_.emplace(std::move(name), std::move(setup));
    return *this;
}

/**
 * @details Scena jest wstawiana do mapy przed wykonaniem konfiguracji, więc konfiguracja
 * może bezpiecznie wywołać Scene() albo SwitchScene() na samą siebie. Węzły std::map nie
 * zmieniają adresu, dlatego referencje do innych scen pozostają ważne.
 */
Scene* Engine::Load(const std::string& name) {
    if (auto it = scenes_.find(name); it != scenes_.end()) return &it->second;

    const auto setup = registered_.find(name);
    if (setup == registered_.end()) {
        Log(LogLevel::Error, "nie ma sceny \"" + name + "\"");
        return nullptr;
    }

    ::Scene& scene = scenes_.try_emplace(name).first->second;
    const double start = ::GetTime();
    setup->second(scene);
    const double elapsed = ::GetTime() - start;
    loadSeconds_ += elapsed;
    Log("Wczytano scene \"" + name + "\" (" + std::to_string(static_cast<int>(elapsed * 1000.0)) +
        " ms)");
    return &scene;
}

/**
 * @details Kolejność rejestracji między plikami jest nieokreślona, dlatego scenę startową
 * należy wskazać przez StartScene(). Bez niej aktywna zostaje pierwsza scena utworzona
 * przez CreateScene(), a gdy takiej nie ma - pierwsza zarejestrowana alfabetycznie.
 */
void Engine::LoadStartScene() {
    if (!start_.empty()) {
        if (Load(start_)) active_ = start_;
        return;
    }
    if (!active_.empty() || registered_.empty()) return;

    active_ = registered_.begin()->first;
    if (registered_.size() > 1)
        Log(LogLevel::Warning, "brak StartScene(), startowa scena: \"" + active_ + "\"");
    Load(active_);
}

Scene& Engine::Scene(const std::string& name) {
    if (::Scene* scene = Load(name)) return *scene;
    throw std::out_of_range("Engine::Scene: nie ma sceny \"" + name + "\"");
}

Engine& Engine::SwitchScene(const std::string& name) {
    if (!scenes_.contains(name) && !registered_.contains(name)) {
        Log(LogLevel::Error, "nie ma sceny \"" + name + "\"");
        return *this;
    }
    // powrót na aktywną scenę anuluje wcześniejsze przełączenie z tej samej klatki
    if (name == active_)
        pending_.clear();
    else
        pending_ = name;
    return *this;
}

Engine& Engine::LoadScene(const std::string& name) {
    if (!initialized_) {
        Log(LogLevel::Error, "LoadScene(\"" + name + "\") przed Init()");
        return *this;
    }
    Load(name);
    return *this;
}

Engine& Engine::UnloadScene(const std::string& name) {
    if (!scenes_.contains(name) && !registered_.contains(name)) {
        Log(LogLevel::Error, "nie ma sceny \"" + name + "\"");
        return *this;
    }
    if (name == active_ && !registered_.contains(name)) {
        Log(LogLevel::Error,
            "aktywnej sceny \"" + name + "\" nie da sie zwolnic (brak rejestracji)");
        return *this;
    }
    unload_.insert(name);
    return *this;
}

Engine& Engine::ReloadScene(const std::string& name) {
    if (!registered_.contains(name)) {
        Log(LogLevel::Error,
            "sceny \"" + name + "\" nie da sie wczytac ponownie (brak rejestracji)");
        return *this;
    }
    reload_.insert(name);
    return *this;
}

/**
 * @details Kolejność:
 * 1. przełączenie na scenę z SwitchScene() (wczytanie, jeśli trzeba),
 * 2. zwolnienie scen z UnloadScene() i ReloadScene(),
 * 3. ponowne wczytanie scen z ReloadScene(),
 * 4. jeśli aktywna scena została zwolniona - wczytanie jej od nowa (restart).
 *
 * Dzięki temu scena może w jednym zdarzeniu przełączyć na inną i zwolnić samą siebie:
 * `engine.SwitchScene("menu").UnloadScene("poziom1");`
 */
void Engine::ApplySceneChanges() {
    if (!pending_.empty()) {
        if (Load(pending_)) {
            active_ = std::move(pending_);
            Log("Scena: " + active_);
        }
        pending_.clear();
    }

    if (unload_.empty() && reload_.empty()) return;

    // kopie: konfiguracja wczytywanej sceny może zlecić kolejne zmiany na następną klatkę
    const auto unload = std::exchange(unload_, {});
    const auto reload = std::exchange(reload_, {});

    for (const auto& names : {std::cref(unload), std::cref(reload)})
        for (const std::string& name : names.get())
            if (scenes_.erase(name) > 0) Log("Zwolniono scene \"" + name + "\"");

    for (const std::string& name : reload)
        Load(name);

    if (!active_.empty() && !scenes_.contains(active_)) Load(active_);
}

Engine& Engine::Clear(const RenderTarget& target, Color color) {
    target.Clear(color);
    return *this;
}

Engine& Engine::Log(LogLevel level, const std::string& message) {
    const std::string line = LevelName(level) + message;
    std::cerr << line << '\n';
    if (log_.is_open()) log_ << line << std::endl;
    return *this;
}

// ---------------------------------------------------------------------------------------------
// Wrappery raylib: wejście (sterowanie), okno, schowek, czas, pliki
// ---------------------------------------------------------------------------------------------

bool Engine::RequireWindow(const char* function) {
    if (initialized_) return true;
    Log(LogLevel::Warning,
        std::string(function) + "() wymaga otwartego okna (po Init()) - pominieto");
    return false;
}

Engine& Engine::ExitKey(int key) {
    exitKey_ = key;
    // przy przejętej klawiaturze klawisz wyjścia jest wyłączony - wróci po utracie fokusu
    if (initialized_ && !input_.IsKeyboardCaptured()) ::SetExitKey(key);
    return *this;
}

Engine& Engine::MousePosition(Vector2 position) {
    if (!RequireWindow("MousePosition")) return *this;
    // pozycja podawana jest we współrzędnych gry; raylib oczekuje pikseli okna
    if (target_.IsValid()) position = input_.GameToWindow(position);
    ::SetMousePosition(static_cast<int>(position.x), static_cast<int>(position.y));
    return *this;
}

Engine& Engine::MouseCursor(int cursor) {
    if (RequireWindow("MouseCursor")) ::SetMouseCursor(cursor);
    return *this;
}

Engine& Engine::HideCursor(bool value) {
    if (!RequireWindow("HideCursor")) return *this;
    if (value)
        ::HideCursor();
    else
        ::ShowCursor();
    return *this;
}

Engine& Engine::LockCursor(bool value) {
    if (!RequireWindow("LockCursor")) return *this;
    if (value)
        ::DisableCursor();
    else
        ::EnableCursor();
    return *this;
}

bool Engine::IsCursorHidden() const {
    return initialized_ && ::IsCursorHidden();
}

bool Engine::IsCursorOnScreen() const {
    return initialized_ && ::IsCursorOnScreen();
}

Engine& Engine::GamepadVibration(int gamepad, float left, float right, float seconds) {
    if (RequireWindow("GamepadVibration")) ::SetGamepadVibration(gamepad, left, right, seconds);
    return *this;
}

int Engine::GamepadMappings(const std::string& mappings) {
    if (!RequireWindow("GamepadMappings")) return 0;
    return ::SetGamepadMappings(mappings.c_str());
}

int Engine::ScreenWidth() const {
    return initialized_ ? ::GetScreenWidth() : 0;
}

int Engine::ScreenHeight() const {
    return initialized_ ? ::GetScreenHeight() : 0;
}

int Engine::RenderWidth() const {
    return initialized_ ? ::GetRenderWidth() : 0;
}

int Engine::RenderHeight() const {
    return initialized_ ? ::GetRenderHeight() : 0;
}

bool Engine::IsWindowResized() const {
    return initialized_ && ::IsWindowResized();
}

bool Engine::IsWindowFocused() const {
    return initialized_ && ::IsWindowFocused();
}

bool Engine::IsWindowHidden() const {
    return initialized_ && ::IsWindowHidden();
}

bool Engine::IsWindowMinimized() const {
    return initialized_ && ::IsWindowMinimized();
}

bool Engine::IsWindowMaximized() const {
    return initialized_ && ::IsWindowMaximized();
}

bool Engine::IsWindowState(unsigned int flag) const {
    return initialized_ && ::IsWindowState(flag);
}

Vector2 Engine::WindowPosition() const {
    return initialized_ ? ::GetWindowPosition() : Vector2{0, 0};
}

Vector2 Engine::WindowScaleDpi() const {
    return initialized_ ? ::GetWindowScaleDPI() : Vector2{1, 1};
}

int Engine::MonitorCount() const {
    return initialized_ ? ::GetMonitorCount() : 0;
}

int Engine::CurrentMonitor() const {
    return initialized_ ? ::GetCurrentMonitor() : 0;
}

Vector2 Engine::MonitorPosition(int monitor) const {
    return initialized_ ? ::GetMonitorPosition(monitor) : Vector2{0, 0};
}

int Engine::MonitorWidth(int monitor) const {
    return initialized_ ? ::GetMonitorWidth(monitor) : 0;
}

int Engine::MonitorHeight(int monitor) const {
    return initialized_ ? ::GetMonitorHeight(monitor) : 0;
}

int Engine::MonitorPhysicalWidth(int monitor) const {
    return initialized_ ? ::GetMonitorPhysicalWidth(monitor) : 0;
}

int Engine::MonitorPhysicalHeight(int monitor) const {
    return initialized_ ? ::GetMonitorPhysicalHeight(monitor) : 0;
}

int Engine::MonitorRefreshRate(int monitor) const {
    return initialized_ ? ::GetMonitorRefreshRate(monitor) : 0;
}

std::string Engine::MonitorName(int monitor) const {
    if (!initialized_) return {};
    const char* name = ::GetMonitorName(monitor);
    return name ? name : "";
}

Engine& Engine::Monitor(int monitor) {
    monitor_ = monitor;
    if (!initialized_) return *this;
    if (monitor < 0 || monitor >= ::GetMonitorCount()) {
        Log(LogLevel::Warning, "nie ma monitora " + std::to_string(monitor) + " - pominieto");
        return *this;
    }
    ::SetWindowMonitor(monitor);
    return *this;
}

Engine& Engine::WindowPosition(int x, int y) {
    windowPosition_ = Vector2{static_cast<float>(x), static_cast<float>(y)};
    if (initialized_) ::SetWindowPosition(x, y);
    return *this;
}

Engine& Engine::Opacity(float opacity) {
    opacity_ = opacity;
    if (initialized_) ::SetWindowOpacity(opacity);
    return *this;
}

Engine& Engine::Maximize() {
    if (RequireWindow("Maximize")) ::MaximizeWindow();
    return *this;
}

Engine& Engine::Minimize() {
    if (RequireWindow("Minimize")) ::MinimizeWindow();
    return *this;
}

Engine& Engine::Restore() {
    if (RequireWindow("Restore")) ::RestoreWindow();
    return *this;
}

Engine& Engine::ToggleBorderless() {
    if (RequireWindow("ToggleBorderless")) ::ToggleBorderlessWindowed();
    return *this;
}

Engine& Engine::Focus() {
    if (RequireWindow("Focus")) ::SetWindowFocused();
    return *this;
}

std::string Engine::ClipboardText() const {
    if (!initialized_) return {};
    const char* text = ::GetClipboardText();
    return text ? text : "";
}

Engine& Engine::ClipboardText(const std::string& text) {
    if (RequireWindow("ClipboardText")) ::SetClipboardText(text.c_str());
    return *this;
}

double Engine::Time() const {
    return initialized_ ? ::GetTime() : 0.0;
}

int Engine::CurrentFps() const {
    return initialized_ ? ::GetFPS() : 0;
}

Engine& Engine::Wait(double seconds) {
    ::WaitTime(seconds);
    return *this;
}

Engine& Engine::RandomSeed(unsigned int seed) {
    ::SetRandomSeed(seed);
    return *this;
}

int Engine::RandomValue(int min, int max) const {
    return ::GetRandomValue(min, max);
}

Engine& Engine::Screenshot(const std::string& path) {
    if (RequireWindow("Screenshot")) ::TakeScreenshot(path.c_str());
    return *this;
}

Engine& Engine::OpenUrl(const std::string& url) {
    ::OpenURL(url.c_str());
    return *this;
}

Engine& Engine::EventWaiting(bool value) {
    if (value)
        ::EnableEventWaiting();
    else
        ::DisableEventWaiting();
    return *this;
}

std::string Engine::ApplicationDirectory() const {
    return ::GetApplicationDirectory();
}

std::string Engine::WorkingDirectory() const {
    return ::GetWorkingDirectory();
}

bool Engine::FileExists(const std::string& path) const {
    return ::FileExists(path.c_str());
}

bool Engine::DirectoryExists(const std::string& path) const {
    return ::DirectoryExists(path.c_str());
}

long Engine::FileModTime(const std::string& path) const {
    return ::GetFileModTime(path.c_str());
}

// ---------------------------------------------------------------------------------------------
// Grupa 2: flagi okna, ikona, logi raylib, pliki, losowość, nagrywanie wejścia
// ---------------------------------------------------------------------------------------------

Engine& Engine::WindowFlag(unsigned int flag, bool value, const char* name, bool onlyBeforeInit) {
    if (value)
        windowFlags_ |= flag;
    else
        windowFlags_ &= ~flag;
    if (!initialized_) return *this;
    if (onlyBeforeInit) {
        Log(LogLevel::Warning, std::string(name) + "() dziala tylko przed Init() - pominieto");
        return *this;
    }
    if (value)
        ::SetWindowState(flag);
    else
        ::ClearWindowState(flag);
    return *this;
}

Engine& Engine::Resizable(bool value) {
    return WindowFlag(FLAG_WINDOW_RESIZABLE, value, "Resizable", false);
}

Engine& Engine::Msaa(bool value) {
    return WindowFlag(FLAG_MSAA_4X_HINT, value, "Msaa", true);
}

Engine& Engine::HighDpi(bool value) {
    return WindowFlag(FLAG_WINDOW_HIGHDPI, value, "HighDpi", true);
}

Engine& Engine::Undecorated(bool value) {
    return WindowFlag(FLAG_WINDOW_UNDECORATED, value, "Undecorated", false);
}

Engine& Engine::AlwaysOnTop(bool value) {
    return WindowFlag(FLAG_WINDOW_TOPMOST, value, "AlwaysOnTop", false);
}

Engine& Engine::MinSize(int width, int height) {
    minSize_ = std::pair{width, height};
    if (initialized_) ::SetWindowMinSize(width, height);
    return *this;
}

Engine& Engine::MaxSize(int width, int height) {
    maxSize_ = std::pair{width, height};
    if (initialized_) ::SetWindowMaxSize(width, height);
    return *this;
}

Engine& Engine::Icon(std::string path) {
    icon_ = std::move(path);
    if (initialized_) ApplyIcon();
    return *this;
}

void Engine::ApplyIcon() {
    if (icon_.empty()) return;
    Image image = ::LoadImage(icon_.c_str());
    if (image.data == nullptr) {
        Log(LogLevel::Error, "nie udalo sie wczytac ikony \"" + icon_ + "\"");
        return;
    }
    // SetWindowIcon wymaga formatu R8G8B8A8
    if (image.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
        ::ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    ::SetWindowIcon(image);
    ::UnloadImage(image);
}

Engine& Engine::RaylibLogLevel(int level) {
    raylibLogLevel_ = level;
    ::SetTraceLogLevel(level);
    return *this;
}

void Engine::PollSystemEvents() {
    if (filesDropped_ && ::IsFileDropped()) {
        const FilePathList files = ::LoadDroppedFiles();
        std::vector<std::string> paths(files.paths, files.paths + files.count);
        ::UnloadDroppedFiles(files);
        filesDropped_(paths);
    }

    if (!replaying_) return;
    // zdarzenia nagrane w klatce N odtwarzamy na początku N-tej klatki odtwarzania
    while (replayIndex_ < replayList_.count &&
           replayList_.events[replayIndex_].frame <= replayFrame_)
        ::PlayAutomationEvent(replayList_.events[replayIndex_++]);
    ++replayFrame_;
    if (replayIndex_ >= replayList_.count) {
        ::UnloadAutomationEventList(replayList_);
        replayList_ = {};
        replaying_ = false;
        Log("Koniec odtwarzania nagrania");
    }
}

Bitmap Engine::ClipboardImage() const {
    if (!initialized_) return {};
    return Bitmap{::GetClipboardImage()};
}

float Engine::RandomFloat(float min, float max) const {
    constexpr int kSteps = 1'000'000;
    const float fraction = static_cast<float>(::GetRandomValue(0, kSteps)) / kSteps;
    return min + (max - min) * fraction;
}

std::vector<int> Engine::RandomSequence(unsigned int count, int min, int max) const {
    int* values = ::LoadRandomSequence(count, min, max);
    if (values == nullptr) return {};
    std::vector<int> result(values, values + count);
    ::UnloadRandomSequence(values);
    return result;
}

std::string Engine::AssetPath(const std::string& name) const {
    return ApplicationDirectory() + "assets/" + name;
}

std::optional<std::string> Engine::LoadText(const std::string& path) const {
    char* text = ::LoadFileText(path.c_str());
    if (text == nullptr) return std::nullopt;
    std::string result = text;
    ::UnloadFileText(text);
    return result;
}

bool Engine::SaveText(const std::string& path, const std::string& text) const {
    std::string copy = text; // raylib przyjmuje char* bez const
    return ::SaveFileText(path.c_str(), copy.data());
}

std::optional<std::vector<unsigned char>> Engine::LoadData(const std::string& path) const {
    int size = 0;
    unsigned char* data = ::LoadFileData(path.c_str(), &size);
    if (data == nullptr) return std::nullopt;
    std::vector<unsigned char> result(data, data + size);
    ::UnloadFileData(data);
    return result;
}

bool Engine::SaveData(const std::string& path, std::span<const unsigned char> data) const {
    // raylib przyjmuje void* bez const, ale danych nie zmienia
    return ::SaveFileData(path.c_str(), const_cast<unsigned char*>(data.data()),
                          static_cast<int>(data.size()));
}

std::vector<std::string> Engine::ListDirectory(const std::string& path, const std::string& filter,
                                               bool recursive) const {
    if (!::DirectoryExists(path.c_str())) return {};
    const FilePathList files =
        ::LoadDirectoryFilesEx(path.c_str(), filter.empty() ? nullptr : filter.c_str(), recursive);
    std::vector<std::string> result(files.paths, files.paths + files.count);
    ::UnloadDirectoryFiles(files);
    return result;
}

bool Engine::MakeDirectory(const std::string& path) const {
    return ::MakeDirectory(path.c_str()) == 0 || ::DirectoryExists(path.c_str());
}

bool Engine::ChangeDirectory(const std::string& path) {
    const bool changed = ::ChangeDirectory(path.c_str());
    if (!changed) Log(LogLevel::Error, "nie udalo sie zmienic katalogu na \"" + path + "\"");
    return changed;
}

Engine& Engine::StartRecording() {
    if (!RequireWindow("StartRecording") || recording_) return *this;
    if (replaying_) {
        Log(LogLevel::Warning, "nie mozna nagrywac podczas odtwarzania - pominieto");
        return *this;
    }
    recordList_ = ::LoadAutomationEventList(nullptr); // pusta lista o pełnej pojemności
    ::SetAutomationEventList(&recordList_);
    ::SetAutomationEventBaseFrame(0); // numery klatek nagrania liczone od zera
    ::StartAutomationEventRecording();
    recording_ = true;
    Log("Nagrywanie wejscia...");
    return *this;
}

bool Engine::StopRecording(const std::string& path) {
    if (!recording_) return false;
    ::StopAutomationEventRecording();
    const bool saved = ::ExportAutomationEventList(recordList_, path.c_str());
    Log(saved ? LogLevel::Info : LogLevel::Error, "Nagranie: " + std::to_string(recordList_.count) +
                                                      " zdarzen -> \"" + path + "\"" +
                                                      (saved ? "" : " - zapis nie powiodl sie"));
    ::UnloadAutomationEventList(recordList_);
    recordList_ = {};
    recording_ = false;
    return saved;
}

bool Engine::Replay(const std::string& path) {
    if (!RequireWindow("Replay")) return false;
    if (recording_) {
        Log(LogLevel::Warning, "nie mozna odtwarzac podczas nagrywania - pominieto");
        return false;
    }
    AutomationEventList list = ::LoadAutomationEventList(path.c_str());
    if (list.count == 0) {
        ::UnloadAutomationEventList(list);
        Log(LogLevel::Error, "brak zdarzen w nagraniu \"" + path + "\"");
        return false;
    }
    if (replaying_) ::UnloadAutomationEventList(replayList_);
    replayList_ = list;
    replayIndex_ = 0;
    replayFrame_ = 0;
    replaying_ = true;
    Log("Odtwarzanie nagrania \"" + path + "\" (" + std::to_string(list.count) + " zdarzen)");
    return true;
}
