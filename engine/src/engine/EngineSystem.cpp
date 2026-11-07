/**
 * @file EngineSystem.cpp
 * @brief Schowek, czas, losowość, zrzuty ekranu i inne usługi systemowe.
 */

#include <raycade/Engine.hpp>

#include <rlgl.h>

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
    // zrzut w środku klatki pokazałby poprzednią albo niedokończoną klatkę - patrz Run()
    if (RequireWindow("Screenshot")) screenshot_ = path;
    return *this;
}

/**
 * @details Wołana w Run() przed EndDrawing(), gdy cała klatka jest już w buforze okna.
 * Własny zapis zamiast TakeScreenshot(): raylib (5.5) obcina ścieżkę do samej nazwy pliku
 * i zapisuje go zawsze w katalogu roboczym.
 */
void Engine::SaveScreenshot() {
    if (screenshot_.empty()) return;
    ::rlDrawRenderBatchActive(); // wierzchołki czekające w buforze rlgl jeszcze nie są w obrazie
    Image image = ::LoadImageFromScreen();
    const bool saved = image.data != nullptr && ::ExportImage(image, screenshot_.c_str());
    ::UnloadImage(image);
    Log(saved ? LogLevel::Info : LogLevel::Error,
        "Zrzut ekranu -> \"" + screenshot_ + "\"" + (saved ? "" : " - zapis nie powiodl sie"));
    screenshot_.clear();
}

Engine& Engine::OpenUrl(const std::string& url) {
    ::OpenURL(url.c_str());
    return *this;
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
