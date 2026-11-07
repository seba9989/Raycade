/**
 * @file EngineFiles.cpp
 * @brief Pliki, katalogi i ścieżki zasobów.
 */

#include <raycade/Engine.hpp>

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
    return ::SaveFileData(
        path.c_str(), const_cast<unsigned char*>(data.data()), static_cast<int>(data.size())
    );
}

std::vector<std::string> Engine::ListDirectory(
    const std::string& path,
    const std::string& filter,
    bool recursive
) const {
    if (!::DirectoryExists(path.c_str())) return {};
    const FilePathList files =
        ::LoadDirectoryFilesEx(path.c_str(), filter.empty() ? nullptr : filter.c_str(), recursive);
    std::vector<std::string> result(files.paths, files.paths + files.count);
    ::UnloadDirectoryFiles(files);
    return result;
}

bool Engine::MakeDirectory(const std::string& path) const {
    // raylib zwraca 0 także wtedy, gdy mkdir się nie udał - wynik sprawdzamy sami
    return ::MakeDirectory(path.c_str()) == 0 && ::DirectoryExists(path.c_str());
}

bool Engine::ChangeDirectory(const std::string& path) const {
    return ::ChangeDirectory(path.c_str()); // przyczynę błędu raylib zapisuje w logu
}
