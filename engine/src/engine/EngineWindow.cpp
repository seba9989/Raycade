/**
 * @file EngineWindow.cpp
 * @brief Ustawienia i stan okna, flagi, ikona i monitory.
 * @details Settery okna przed Init() tylko zapisują ustawienia, a po Init() zmieniają okno
 * na bieżąco.
 */

#include <raycade/Engine.hpp>

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

Engine& Engine::Fps(int fps) {
    fps_ = fps;
    if (initialized_) ::SetTargetFPS(fps_);
    return *this;
}

Engine& Engine::VSync(bool value) {
    return WindowFlag(FLAG_VSYNC_HINT, value, "VSync", false);
}

bool Engine::IsVSync() const {
    return (windowFlags_ & FLAG_VSYNC_HINT) != 0;
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
