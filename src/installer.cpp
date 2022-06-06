#include "installer.h"

#include "settings.h"
#include "global.h"

#include <filesystem>

void Installer::init() {
    auto path = std::filesystem::path(".");

    if (!std::filesystem::exists(path / "settings.ini"))
        return;

    if (!(std::filesystem::exists(path / Global::settings.psarcPath) && std::filesystem::is_directory(path / Global::settings.psarcPath)))
        return;

    Global::isInstalled = true;
}

void Installer::install() {
    auto path = std::filesystem::path(".");
    
    if (!(std::filesystem::exists(path / Global::settings.psarcPath) && std::filesystem::is_directory(path / Global::settings.psarcPath)))
        std::filesystem::create_directory((path / Global::settings.psarcPath).string().c_str());

#ifdef SUPPORT_VST
    if (!(std::filesystem::exists(path / Global::settings.vstPath) && std::filesystem::is_directory(path / Global::settings.vstPath)))
      std::filesystem::create_directory((path / Global::settings.vstPath).string().c_str());
#endif // SUPPORT_VST

    Global::isInstalled = true;
}
