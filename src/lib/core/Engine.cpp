/**
 * @file Engine.cpp
 * @brief Inicjalizacja okna, pętla gry, obsługa scen i diagnostyka.
 * @details Settery okna przed Init() tylko zapisują ustawienia, a po Init() zmieniają
 * okno na bieżąco.
 */

#include "Engine.hpp"

#include <iostream>

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
    if (initialized_ && ::IsWindowFullscreen() != value) ::ToggleFullscreen();
    return *this;
}

Engine& Engine::VSync(bool value) {
    vsync_ = value;
    if (initialized_) {
        if (value)
            ::SetWindowState(FLAG_VSYNC_HINT);
        else
            ::ClearWindowState(FLAG_VSYNC_HINT);
    }
    return *this;
}

Engine& Engine::Fps(int fps) {
    fps_ = fps;
    if (initialized_) ::SetTargetFPS(fps_);
    return *this;
}

Engine& Engine::Init() {
    if (initialized_) return *this;
    if (!logFile_.empty()) log_.open(logFile_);

    // flagi okna muszą być ustawione przed InitWindow
    unsigned int flags = 0;
    if (fullscreen_) flags |= FLAG_FULLSCREEN_MODE;
    if (vsync_) flags |= FLAG_VSYNC_HINT;
    ::SetConfigFlags(flags);

    if (fullscreen_)
        ::InitWindow(0, 0, title_.c_str());
    else
        ::InitWindow(width_, height_, title_.c_str());

    if (!::IsWindowReady()) {
        Log("BLAD: nie udalo sie utworzyc okna");
        return *this;
    }

    ::SetTargetFPS(fps_);
    initialized_ = true;
    Log("Silnik zainicjalizowany (podwojne buforowanie, V-Sync: " +
        std::string(vsync_ ? "tak" : "nie") + ")");
    return *this;
}

int Engine::Run() {
    if (!Init().Ready()) return 1;
    CreateRegisteredScenes();

    while (!::WindowShouldClose()) {
        const float dt = ::GetFrameTime();
        Scene* scene = active_.empty() ? nullptr : &scenes_.at(active_);

        if (update_) update_(dt);
        if (scene) scene->Update(dt, input_);

        ::BeginDrawing();
        ::ClearBackground(background_);
        if (scene) scene->Draw(primitives_);
        if (draw_) draw_(primitives_.Reset().TargetScreen());
        ::EndDrawing();

        if (!pending_.empty()) {
            active_ = std::move(pending_);
            pending_.clear();
            Log("Scena: " + active_);
        }
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
    ::CloseWindow();
    initialized_ = false;
}

Scene& Engine::CreateScene(const std::string& name) {
    auto [it, created] = scenes_.try_emplace(name);
    if (!created) Log("UWAGA: scena \"" + name + "\" juz istnieje");
    if (active_.empty()) active_ = name;
    return it->second;
}

Engine& Engine::CreateScene(const std::string& name, const std::function<void(Scene&)>& setup) {
    setup(CreateScene(name));
    return *this;
}

Engine& Engine::RegisterScene(std::string name, std::function<void(Scene&)> setup) {
    registered_[std::move(name)] = std::move(setup);
    return *this;
}

/**
 * @details Kolejność rejestracji między plikami jest nieokreślona, dlatego scena startowa
 * pochodzi z StartScene(), a nie z tego, która zarejestrowała się pierwsza. Brak sceny
 * startowej jest zapisywany w logu.
 */
void Engine::CreateRegisteredScenes() {
    for (auto& [name, setup] : registered_)
        setup(CreateScene(name));
    registered_.clear();

    if (!start_.empty()) {
        if (scenes_.contains(start_))
            active_ = start_;
        else
            Log("BLAD: nie ma sceny startowej \"" + start_ + "\"");
    } else if (scenes_.size() > 1) {
        Log("UWAGA: brak StartScene(), startowa scena: \"" + active_ + "\"");
    }
}

Engine& Engine::SwitchScene(const std::string& name) {
    if (!scenes_.contains(name)) {
        Log("BLAD: nie ma sceny \"" + name + "\"");
        return *this;
    }
    if (name != active_) pending_ = name;
    return *this;
}

Engine& Engine::Clear(RenderTexture2D& target, Color color) {
    ::BeginTextureMode(target);
    ::ClearBackground(color);
    ::EndTextureMode();
    return *this;
}

Engine& Engine::Log(const std::string& message) {
    std::cerr << message << '\n';
    if (log_.is_open()) log_ << message << std::endl;
    return *this;
}
