/**
 * @file Engine.cpp
 * @brief Inicjalizacja okna, pętla gry, obsługa scen i diagnostyka.
 * @details Settery okna przed Init() tylko zapisują ustawienia, a po Init() zmieniają
 * okno na bieżąco.
 */

#include <raycade/core/Engine.hpp>

#include <algorithm>
#include <iostream>
#include <stdexcept>

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
    LoadStartScene();
    double previous = ::GetTime();
    loadSeconds_ = 0.0;

    while (!::WindowShouldClose()) {
        // Własny pomiar zamiast GetFrameTime(): raylib wlicza czas wczytywania sceny do klatki
        // dopiero z opóźnieniem. Odejmujemy go, żeby obiekty nie "skoczyły" o duże dt.
        const double now = ::GetTime();
        const float dt = static_cast<float>(std::max(0.0, now - previous - loadSeconds_));
        previous = now;
        loadSeconds_ = 0.0;
        const auto found = scenes_.find(active_);
        Scene* scene = found != scenes_.end() ? &found->second : nullptr;

        if (update_) update_(dt);
        if (scene) scene->Update(dt, input_);

        ::BeginDrawing();
        ::ClearBackground(background_);
        if (scene) scene->Draw(primitives_);
        if (draw_) draw_(primitives_.Reset().TargetScreen());
        ::EndDrawing();

        // zmiany scen między klatkami, poza pętlą po obiektach
        ApplySceneChanges();
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
    // przed main() plik logu nie jest jeszcze otwarty - komunikat trafi tylko na stderr
    if (registered_.contains(name)) {
        Log("UWAGA: scena \"" + name + "\" zarejestrowana ponownie - pominieto");
        return *this;
    }
    registered_.emplace(std::move(name), std::move(setup));
    return *this;
}

/**
 * @details Scena jest wstawiana do mapy przed wykonaniem konfiguracji, więc konfiguracja
 * może bezpiecznie wywołać GetScene() albo SwitchScene() na samą siebie. Węzły std::map nie
 * zmieniają adresu, dlatego referencje do innych scen pozostają ważne.
 */
Scene* Engine::Load(const std::string& name) {
    if (auto it = scenes_.find(name); it != scenes_.end()) return &it->second;

    const auto setup = registered_.find(name);
    if (setup == registered_.end()) {
        Log("BLAD: nie ma sceny \"" + name + "\"");
        return nullptr;
    }

    Scene& scene = scenes_.try_emplace(name).first->second;
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
        Log("UWAGA: brak StartScene(), startowa scena: \"" + active_ + "\"");
    Load(active_);
}

Scene& Engine::GetScene(const std::string& name) {
    if (Scene* scene = Load(name)) return *scene;
    throw std::out_of_range("Engine::GetScene: nie ma sceny \"" + name + "\"");
}

Engine& Engine::SwitchScene(const std::string& name) {
    if (!scenes_.contains(name) && !registered_.contains(name)) {
        Log("BLAD: nie ma sceny \"" + name + "\"");
        return *this;
    }
    if (name != active_) pending_ = name;
    return *this;
}

Engine& Engine::LoadScene(const std::string& name) {
    if (!initialized_) {
        Log("BLAD: LoadScene(\"" + name + "\") przed Init()");
        return *this;
    }
    Load(name);
    return *this;
}

Engine& Engine::UnloadScene(const std::string& name) {
    if (!scenes_.contains(name) && !registered_.contains(name)) {
        Log("BLAD: nie ma sceny \"" + name + "\"");
        return *this;
    }
    if (name == active_ && !registered_.contains(name)) {
        Log("BLAD: aktywnej sceny \"" + name + "\" nie da sie zwolnic (brak rejestracji)");
        return *this;
    }
    unload_.insert(name);
    return *this;
}

Engine& Engine::ReloadScene(const std::string& name) {
    if (!registered_.contains(name)) {
        Log("BLAD: sceny \"" + name + "\" nie da sie wczytac ponownie (brak rejestracji)");
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
