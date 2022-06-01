#include "profile.h"

#include "vst.h"
#include "global.h"
#include "file.h"

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
        { std::string("Score_") + Global::profileName, std::to_string(manifestInfo.score) },
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
        { std::string("Score_") + Global::profileName, std::to_string(manifestInfo.score) },
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
  char profileIni[sizeof("profile_") + NUM(Global::profileName) + sizeof(".ini")] = "profile_";
  strcat(profileIni, Global::profileName);
  strcat(profileIni, ".ini");

  if (!File::exists(profileIni))
    return;

  const std::map<std::string, std::map<std::string, std::string>> serializedSaves = File::loadIni(profileIni);

  for (Song::Info& songInfo : Global::songInfos)
  {
    for (Manifest::Info& manifestInfo : songInfo.manifestInfos)
    {
      const auto search = serializedSaves.find(manifestInfo.persistentID);
      if (search != serializedSaves.end())
      {
        manifestInfo.lastPlayed = strtoul(search->second.at("LastPlayed").c_str(), nullptr, 0);
        manifestInfo.score = stof(search->second.at(std::string("Score_") + Global::profileName));
      }
    }
  }
}

void Profile::init()
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

static std::map<i32, std::string> tone(InstrumentFlags instrumentFlags)
{
  std::map<i32, std::string> toneAssignment;

  for (i32 i = 0; i < NUM(Global::effectChain); ++i)
  {
    if (Global::effectChain[i] < 0)
      continue;

    toneAssignment.insert({ i, Vst::saveParameters(i) });
  }

  return toneAssignment;
}

static void saveStatsOnly()
{
  std::map<std::string, std::map<std::string, std::string>> serializedSaves;

  std::map<std::string, std::string> toneAssignment;
  toneAssignment.insert({ "0B", "Parameter" });
  for (const auto&[ index, paramters ] : tone(InstrumentFlags::LeadGuitar))
  {
    toneAssignment.insert({ std::string("0G") + std::to_string(index), paramters });
  }
  toneAssignment.insert({ "1B", "Parameter" });
  toneAssignment.insert({ "1G", "Parameter" });
  toneAssignment.insert({ "2B", "Parameter" });
  toneAssignment.insert({ "2G", "Parameter" });
  toneAssignment.insert({ "3B", "Parameter" });
  toneAssignment.insert({ "3G", "Parameter" });
  toneAssignment.insert({ "4B", "Parameter" });
  toneAssignment.insert({ "4G", "Parameter" });
  toneAssignment.insert({ "5B", "Parameter" });
  toneAssignment.insert({ "5G", "Parameter" });
  toneAssignment.insert({ "6B", "Parameter" });
  toneAssignment.insert({ "6G", "Parameter" });
  toneAssignment.insert({ "7B", "Parameter" });
  toneAssignment.insert({ "7G", "Parameter" });
  toneAssignment.insert({ "8B", "Parameter" });
  toneAssignment.insert({ "8G", "Parameter" });
  toneAssignment.insert({ "9B", "Parameter" });
  toneAssignment.insert({ "9G", "Parameter" });
  serializedSaves.insert({ "!ToneAssignment", toneAssignment });
  for (i32 i = 0; i < NUM(Global::effectChain); ++i)
  {

  }
  for (const Song::Info& songInfo : Global::songInfos)
  {
    const std::map<std::string, std::map<std::string, std::string>> map = serialize(songInfo.manifestInfos);
    serializedSaves.insert(map.begin(), map.end());
  }

  char profileIni[sizeof("profile_") + NUM(Global::profileName) + sizeof(".ini")] = "profile_";
  strcat(profileIni, Global::profileName);
  strcat(profileIni, ".ini");

  File::saveIni(profileIni, serializedSaves);
}

static void saveWholeManifest()
{
  std::map<std::string, std::map<std::string, std::string>> serializedSaves;

  for (const Song::Info& songInfo : Global::songInfos)
  {
    const std::map<std::string, std::map<std::string, std::string>> map = serialize(songInfo.manifestInfos);
    serializedSaves.insert(map.begin(), map.end());
  }

  char profileIni[sizeof("profile_") + NUM(Global::profileName) + sizeof(".ini")] = "profile_";
  strcat(profileIni, Global::profileName);
  strcat(profileIni, ".ini");

  File::saveIni(profileIni, serializedSaves);
}

void Profile::fini()
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