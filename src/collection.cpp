#include "collection.h"

#include "psarc.h"
#include "song.h"
#include "global.h"

#include <filesystem>
#include <thread>

#ifdef COLLECTION_WORKER_THREAD
static void fillCollection()
{
  for (const auto& file : std::filesystem::directory_iterator(std::filesystem::path(Global::settings.psarcPath))) {
    if (file.path().extension() != std::filesystem::path(".psarc"))
      continue;

    const Psarc::Info psarcInfo = Psarc::parse(Psarc::readPsarcData(file.path().string().c_str()));
    const Song::Info songInfo = Song::loadSongInfoManifestOnly(psarcInfo);

    {
      const std::unique_lock lock(Global::psarcInfosMutex);
      Global::psarcInfos.emplace_back(std::move(psarcInfo));
      Global::songInfos.emplace_back(std::move(songInfo));
    }
  }
}
#endif // COLLECTION_WORKER_THREAD

void Collection::init()
{
  if (!Global::isInstalled)
    return;

#ifdef COLLECTION_WORKER_THREAD
  static std::thread wokerThread(fillCollection);
#else // COLLECTION_WORKER_THREAD
  for (const auto& file : std::filesystem::directory_iterator(std::filesystem::path(Global::settings.psarcPath)))
  {
    if (file.path().extension() != std::filesystem::path(".psarc"))
      continue;
    Global::psarcInfos.push_back(Psarc::parse(Psarc::readPsarcData(file.path().string().c_str())));
  }

  Global::songInfos.resize(Global::psarcInfos.size());
  for (i32 i = 0; i < Global::psarcInfos.size(); ++i)
  {
    Global::songInfos[i] = Song::loadSongInfoManifestOnly(Global::psarcInfos[i]);
  }
#endif // COLLECTION_WORKER_THREAD
}