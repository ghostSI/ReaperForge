#include "collection.h"

#include "psarc.h"
#include "song.h"
#include "global.h"

#include <filesystem>

void Collection::init() {

  Global::collection.clear();

  if (!Global::isInstalled)
    return;

  for (const auto& file : std::filesystem::directory_iterator(std::filesystem::path("songs"))) {
    if (file.path().extension() != std::filesystem::path(".psarc"))
      continue;

    Global::psarcInfos.push_back(Psarc::parse(Psarc::readPsarcData(file.path().string().c_str())));
    Global::collection.push_back(Song::loadSongInfoManifestOnly(Global::psarcInfos[Global::psarcInfos.size() - 1]));
  }
}