#include "installer.h"

#include "settings.h"

#include <filesystem>

bool Installer::isInstalled(const char *installPath) {
    auto path = std::filesystem::path(installPath);

    if (!std::filesystem::exists(path / "settings.ini"))
        return false;


    if (!(std::filesystem::exists(path / "songs") && std::filesystem::is_directory(path / "songs")))
        return false;

    return true;
}

void Installer::install(const char *installPath) {
    auto path = std::filesystem::path(installPath);

    if (!std::filesystem::exists(path / "settings.ini"))
        Settings::save();

    if (!(std::filesystem::exists(path / "songs") && std::filesystem::is_directory(path / "songs")))
        std::filesystem::create_directory((path / "songs").string().c_str());
}
