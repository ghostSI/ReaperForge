#include "installer.h"

#include "settings.h"
#include "global.h"

#include <filesystem>

void Installer::init() {
    auto path = std::filesystem::path(".");

    if (!std::filesystem::exists(path / "settings.ini"))
        return;


    if (!(std::filesystem::exists(path / "songs") && std::filesystem::is_directory(path / "songs")))
        return;

    Global::isInstalled = true;
}

void Installer::install() {
    auto path = std::filesystem::path(".");

    if (!(std::filesystem::exists(path / "songs") && std::filesystem::is_directory(path / "songs")))
        std::filesystem::create_directory((path / "songs").string().c_str());

    Global::isInstalled = true;
}
