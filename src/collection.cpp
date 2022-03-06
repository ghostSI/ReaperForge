#include "collection.h"

#include "file.h"
#include "installer.h"
#include "psarc.h"
#include "song.h"
#include "global.h"

#include <filesystem>

void Collection::init() {

    if (Global::collectionLoaded)
        return;

    Global::collection.clear();

    if (!Installer::isInstalled("."))
        return;

    for (const auto &file: std::filesystem::directory_iterator(std::filesystem::path("songs"))) {
        if (file.path().extension() != std::filesystem::path(".psarc"))
            continue;

        const std::vector<u8> psarcData = Psarc::readPsarcData(file.path().string().c_str());
        const Psarc::PsarcInfo psarcInfo = Psarc::parse(psarcData);
        const Song::Info songInfo = Song::psarcInfoToSongInfo(psarcInfo);

        Global::collection.push_back(songInfo);
    }

    Global::collectionLoaded = true;
}