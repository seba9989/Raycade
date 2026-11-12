/**
 * @file Engine.cpp
 * @brief Inicjalizacja, pętla gry, zamykanie i log silnika.
 * @details Pozostałe metody Engine są w plikach Engine*.cpp w tym katalogu, pogrupowane
 * jak w dokumentacji klasy: okno (EngineWindow.cpp), skalowanie (EngineScaling.cpp), sceny
 * (EngineScenes.cpp), wejście (EngineInput.cpp), pliki (EngineFiles.cpp) oraz schowek, czas
 * i losowość (EngineSystem.cpp).
 */

#include <raycade/Engine.hpp>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <iostream>

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
        ::SetWindowPosition(
            static_cast<int>(windowPosition_->x), static_cast<int>(windowPosition_->y)
        );
    if (opacity_) ::SetWindowOpacity(*opacity_);
    if (minSize_) ::SetWindowMinSize(minSize_->first, minSize_->second);
    if (maxSize_) ::SetWindowMaxSize(maxSize_->first, maxSize_->second);
    initialized_ = true;
    SetContext(assets_); // Shutdown() czyści kontekst - po ponownym Init() wpis wraca
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
                if (scene) scene->FixedUpdate(fixedStep_, input_);
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
        // RenderTarget, bo tekstury obiektów (visual::RenderGroup) muszą wracać do niej, nie na
        // ekran
        if (scaled)
            target_.Begin();
        else
            ::BeginDrawing();
        ::ClearBackground(background_);
        if (scene) {
            // położenie obiektów fizyki między krokami stałymi (interpolacja rysowania)
            scene->FixedAlpha(fixedStep_ > 0.0f ? FixedAlpha() : 1.0f);
            scene->Draw(primitives_);
        }
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
        SaveScreenshot();
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
    ClearContext();  // wartości globalne też mogą trzymać tekstury
    assets_.Clear(); // na końcu: sceny i kontekst mogły trzymać zasoby z pamięci podręcznej
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

bool Engine::RequireWindow(const char* function) {
    if (initialized_) return true;
    Log(LogLevel::Warning,
        std::string(function) + "() wymaga otwartego okna (po Init()) - pominieto");
    return false;
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
