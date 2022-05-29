#include "installer.h"

#include "settings.h"
#include "global.h"

#include <filesystem>

void Installer::init() {
    auto path = std::filesystem::path(".");

    if (!std::filesystem::exists(path / "settings.ini"))
        return;


    if (!(std::filesystem::exists(path / "psarc") && std::filesystem::is_directory(path / "psarc")))
        return;

    Global::isInstalled = true;
}

void Installer::install() {
    auto path = std::filesystem::path(".");

    if (!(std::filesystem::exists(path / "psarc") && std::filesystem::is_directory(path / "psarc")))
        std::filesystem::create_directory((path / "psarc").string().c_str());

    Global::isInstalled = true;
}
