#include "saves.h"

#include "file.h"
#include "global.h"

static std::map<std::string, std::map<std::string, std::string>> serialize(const std::vector<Manifest::Info::Entry>& saveInfos)
{
  std::map<std::string, std::map<std::string, std::string>> serializedSaves;

  switch (Global::settings.saveMode)
  {
  case SaveMode::statsOnly:
    for (const auto& saveInfo : saveInfos)
    {
      std::map<std::string, std::string> serializedSave =
      {
        { "LastPlayed", std::to_string(saveInfo.lastPlayed) },
        { std::string("Score_") + Global::playerName, std::to_string(saveInfo.score) },
      };

      serializedSaves.insert({ saveInfo.persistentID, serializedSave });
    }
    break;
  case SaveMode::wholeManifest:
    for (const auto& saveInfo : saveInfos)
    {
      std::map<std::string, std::string> serializedSave =
      {
        { "AlbumArt", saveInfo.albumArt },
        { "AlbumName", saveInfo.albumName },
        { "ArrangementName", saveInfo.arrangementName },
        { "ArtistName", saveInfo.artistName },
        { "BassPick", std::to_string(saveInfo.bassPick) },
        { "CapoFret", std::to_string(saveInfo.capoFret) },
        { "CentOffset", std::to_string(saveInfo.centOffset) },
        { "LastPlayed", std::to_string(saveInfo.lastPlayed) },
        { std::string("Score_") + Global::playerName, std::to_string(saveInfo.score) },
        { "SongLength", std::to_string(saveInfo.songLength) },
        { "SongName", saveInfo.songName },
        { "SongYear", std::to_string(saveInfo.songYear) },
        { "Tuning", std::to_string(saveInfo.tuning.string0) + ',' + std::to_string(saveInfo.tuning.string1) + ',' + std::to_string(saveInfo.tuning.string2) + ',' + std::to_string(saveInfo.tuning.string3) + ',' + std::to_string(saveInfo.tuning.string4) + ',' + std::to_string(saveInfo.tuning.string5)}
      };

      serializedSaves.insert({ saveInfo.persistentID, serializedSave });
    }
    break;
  }

  return serializedSaves;
}

static void saveStatsOnly()
{
  std::map<std::string, std::map<std::string, std::string>> serializedSaves;

  for (const Song::Info& songInfo : Global::collection)
  {
    const std::map<std::string, std::map<std::string, std::string>> map = serialize(songInfo.manifest.entries);
    serializedSaves.insert(map.begin(), map.end());
  }

  File::saveIni("saves.ini", serializedSaves);
}

static void saveWholeManifest()
{
  std::map<std::string, std::map<std::string, std::string>> serializedSaves;

  for (const Song::Info& songInfo : Global::collection)
  {
    const std::map<std::string, std::map<std::string, std::string>> map = serialize(songInfo.manifest.entries);
    serializedSaves.insert(map.begin(), map.end());
  }

  File::saveIni("saves.ini", serializedSaves);
}

void Saves::save()
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

static void loadStatsOnly()
{
  const std::map<std::string, std::map<std::string, std::string>> serializedSaves = File::loadIni("saves.ini");

  for (Song::Info& songInfo : Global::collection)
  {
    for (Manifest::Info::Entry& manifestEntry : songInfo.manifest.entries)
    {
      const auto search = serializedSaves.find(manifestEntry.persistentID);
      if (search != serializedSaves.end())
      {
        manifestEntry.lastPlayed = strtoul(search->second.at("LastPlayed").c_str(), nullptr, 0);
        manifestEntry.score = stof(search->second.at(std::string("Score_") + Global::playerName));
      }
    }
  }
}

std::vector<Manifest::Info> Saves::load()
{
  std::vector<Manifest::Info> saveInfos;

  switch (Global::settings.saveMode)
  {
  case SaveMode::statsOnly:
    loadStatsOnly();
    break;
  case SaveMode::wholeManifest:
    //loadWholeManifest();
    break;
  }

  return saveInfos;
}