/**
 * @file EngineScenes.cpp
 * @brief Tworzenie, rejestracja, leniwe wczytywanie i odroczone przełączanie scen.
 */

#include <raycade/Engine.hpp>

#include <stdexcept>

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
 * 4. jeśli aktywna scena została zwolniona - wczytanie jej od nowa (restart),
 * 5. zwolnienie zasobów (Assets::Purge()), których nie używa już żadna scena.
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

    // dopiero po wczytaniu: zasób użyty także przez nową scenę zostaje w pamięci
    if (const std::size_t released = assets_.Purge(); released > 0)
        Log("Zwolniono nieuzywane zasoby: " + std::to_string(released));
}
