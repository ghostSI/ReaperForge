#include "collection.h"

#include "psarc.h"
#include "song.h"
#include "global.h"

#include <filesystem>

void Collection::init() {

  if (!Global::isInstalled)
    return;

  for (const auto& file : std::filesystem::directory_iterator(std::filesystem::path("psarc"))) {
    if (file.path().extension() != std::filesystem::path(".psarc"))
      continue;
    Global::psarcInfos.push_back(Psarc::parse(Psarc::readPsarcData(file.path().string().c_str())));
  }

  Global::songInfos.resize(Global::psarcInfos.size());
  for (i32 i = 0; i < Global::psarcInfos.size(); ++i) {
    Global::songInfos[i] = Song::loadSongInfoManifestOnly(Global::psarcInfos[i]);
  }
}