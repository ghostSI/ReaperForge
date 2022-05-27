#include "saves.h"

#include "file.h"
#include "global.h"

static std::map<std::string, std::map<std::string, std::string>> serialize(const std::vector<Manifest::Info>& manifestInfos)
{
  std::map<std::string, std::map<std::string, std::string>> serializedSaves;

  switch (Global::settings.saveMode)
  {
  case SaveMode::statsOnly:
    for (const auto& manifestInfo : manifestInfos)
    {
      std::map<std::string, std::string> serializedSave =
      {
        { "LastPlayed", std::to_string(manifestInfo.lastPlayed) },
        { std::string("Score_") + Global::playerName, std::to_string(manifestInfo.score) },
      };

      serializedSaves.insert({ manifestInfo.persistentID, serializedSave });
    }
    break;
  case SaveMode::wholeManifest:
    for (const auto& manifestInfo : manifestInfos)
    {
      std::map<std::string, std::string> serializedSave =
      {
        { "AlbumArt", manifestInfo.albumArt },
        { "AlbumName", manifestInfo.albumName },
        { "ArrangementName", manifestInfo.arrangementName },
        { "ArtistName", manifestInfo.artistName },
        { "BassPick", std::to_string(manifestInfo.bassPick) },
        { "CapoFret", std::to_string(manifestInfo.capoFret) },
        { "CentOffset", std::to_string(manifestInfo.centOffset) },
        { "LastPlayed", std::to_string(manifestInfo.lastPlayed) },
        { std::string("Score_") + Global::playerName, std::to_string(manifestInfo.score) },
        { "SongLength", std::to_string(manifestInfo.songLength) },
        { "SongName", manifestInfo.songName },
        { "SongYear", std::to_string(manifestInfo.songYear) },
        { "Tuning", std::to_string(manifestInfo.tuning.string[0]) + ',' + std::to_string(manifestInfo.tuning.string[1]) + ',' + std::to_string(manifestInfo.tuning.string[2]) + ',' + std::to_string(manifestInfo.tuning.string[3]) + ',' + std::to_string(manifestInfo.tuning.string[4]) + ',' + std::to_string(manifestInfo.tuning.string[5])}
      };

      serializedSaves.insert({ manifestInfo.persistentID, serializedSave });
    }
    break;
  }

  return serializedSaves;
}

static void loadStatsOnly()
{
  const std::map<std::string, std::map<std::string, std::string>> serializedSaves = File::loadIni("saves.ini");

  for (Song::Info& songInfo : Global::songInfos)
  {
    for (Manifest::Info& manifestInfo : songInfo.manifestInfos)
    {
      const auto search = serializedSaves.find(manifestInfo.persistentID);
      if (search != serializedSaves.end())
      {
        manifestInfo.lastPlayed = strtoul(search->second.at("LastPlayed").c_str(), nullptr, 0);
        manifestInfo.score = stof(search->second.at(std::string("Score_") + Global::playerName));
      }
    }
  }
}

void Saves::init()
{
  switch (Global::settings.saveMode)
  {
  case SaveMode::statsOnly:
    loadStatsOnly();
    break;
  case SaveMode::wholeManifest:
    //loadWholeManifest();
    break;
  }
}

static void saveStatsOnly()
{
  std::map<std::string, std::map<std::string, std::string>> serializedSaves;

  for (const Song::Info& songInfo : Global::songInfos)
  {
    const std::map<std::string, std::map<std::string, std::string>> map = serialize(songInfo.manifestInfos);
    serializedSaves.insert(map.begin(), map.end());
  }

  File::saveIni("saves.ini", serializedSaves);
}

static void saveWholeManifest()
{
  std::map<std::string, std::map<std::string, std::string>> serializedSaves;

  for (const Song::Info& songInfo : Global::songInfos)
  {
    const std::map<std::string, std::map<std::string, std::string>> map = serialize(songInfo.manifestInfos);
    serializedSaves.insert(map.begin(), map.end());
  }

  File::saveIni("saves.ini", serializedSaves);
}

void Saves::fini()
{
  switch (Global::settings.saveMode)
  {
  case SaveMode::statsOnly:
    saveStatsOnly();
    break;
  case SaveMode::wholeManifest:
    saveWholeManifest();
    break;
  }
}