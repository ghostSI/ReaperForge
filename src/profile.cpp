#include "profile.h"

#include "file.h"
#include "global.h"
#include "plugin.h"

#include <string.h>

static std::map<std::string, std::map<std::string, std::string>> serialize(const std::vector<Manifest::Info>& manifestInfos)
{
  std::map<std::string, std::map<std::string, std::string>> serializedSaves;

  switch (Global::settings.profileSaveMode)
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

#ifdef SUPPORT_PLUGIN
static std::string toneAssignmentNames[2][Const::profileToneAssignmentCount] =
{
  {
    "Default"
  },
  {
    "Default"
  }
};
static i32 toneAssignmentIndex[2][Const::profileToneAssignmentCount][NUM(Global::effectChain)] =
{
  {
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) }
  },
  {
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) },
    { ARR_SET16(-1) }
  }
};
static std::string toneAssignmentBase64[2][Const::profileToneAssignmentCount][NUM(Global::effectChain)];
#endif // SUPPORT_PLUGIN

static void loadStatsOnly()
{
  char profileIni[sizeof("profile_") + NUM(Global::profileName) + sizeof(".ini")] = "profile_";
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
  strcat(profileIni, Global::profileName);
  strcat(profileIni, ".ini");
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32

  if (!File::exists(profileIni))
    return;

  const std::map<std::string, std::map<std::string, std::string>> serializedSaves = File::loadIni(profileIni);

#ifdef SUPPORT_PLUGIN
  const auto toneAssignmentIt = serializedSaves.find("!ToneAssignment");
  if (toneAssignmentIt != serializedSaves.end())
  {
    const std::map<std::string, std::string>& toneAssignmentTime = toneAssignmentIt->second;

    for (i32 h = 0; h < 2; ++h)
    {
      const char instChar = h == 0 ? 'B' : 'G';

      for (i32 i = 0; i < Const::profileToneAssignmentCount; ++i)
      {
        if (const auto search3 = toneAssignmentTime.find(std::to_string(i) + instChar); search3 != toneAssignmentTime.end())
        {
          toneAssignmentNames[h][i] = search3->second;
        }

        for (i32 j = 0; j < NUM(Global::effectChain); ++j)
        {
          toneAssignmentIndex[h][i][j] = -1;
          const auto search2 = toneAssignmentTime.find(std::to_string(i) + instChar + std::to_string(j));
          if (search2 != toneAssignmentTime.end())
          {
            const u64 seperator = search2->second.find_last_of(';');
            assert(seperator != std::string::npos);
            const std::string pluginName = search2->second.substr(0, seperator);
            if (!pluginName.empty())
            {
              for (i32 k = 0; k < Global::pluginNames.size(); ++k)
              {
                if (pluginName == Global::pluginNames[k])
                {
                  toneAssignmentIndex[h][i][j] = k;
                  const std::string base64 = search2->second.substr(seperator + 1);
                  toneAssignmentBase64[h][i][j] = base64;
                  break;
                }
              }
            }
          }
        }
      }
    }
  }

  { // Load Tone Assignment 0
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
    strcpy(Global::vstToneName, toneAssignmentNames[1][0].c_str());
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32
    Global::vstToneNameLength = i32(toneAssignmentNames[1][0].size());

    for (i32 j = 0; j < NUM(Global::effectChain); ++j)
    {
      Global::effectChain[j] = toneAssignmentIndex[1][0][j];
      if (Global::effectChain[j] >= 0)
      {
        i32 instance = 0;
        for (i32 k = 0; k < j; ++k)
          if (Global::effectChain[j] == Global::effectChain[k])
            ++instance;

        Plugin::loadParameter(Global::effectChain[j], instance, toneAssignmentBase64[1][0][j]);
      }
    }
  }
#endif // SUPPORT_PLUGIN


  for (Song::Info& songInfo : Global::songInfos)
  {
    for (Manifest::Info& manifestInfo : songInfo.manifestInfos)
    {
      const auto persistentIdIt = serializedSaves.find(manifestInfo.persistentID);
      if (persistentIdIt != serializedSaves.end())
      {
        manifestInfo.lastPlayed = strtoul(persistentIdIt->second.at("LastPlayed").c_str(), nullptr, 0);
        manifestInfo.score = stof(persistentIdIt->second.at(std::string("Score_") + Global::profileName));
      }
    }
  }
}

void Profile::init()
{
  switch (Global::settings.profileSaveMode)
  {
  case SaveMode::statsOnly:
    loadStatsOnly();
    break;
  case SaveMode::wholeManifest:
    //loadWholeManifest();
    break;
  }
}

void Profile::tick()
{
#ifdef SUPPORT_PLUGIN
  static InstrumentFlags instrumentLastFrame = Global::currentInstrument;
  static i32 toneAssignmentLastFrame = Global::toneAssignment;

  if (instrumentLastFrame != Global::currentInstrument || toneAssignmentLastFrame != Global::toneAssignment)
  {
    const i32 h = Global::currentInstrument == InstrumentFlags::BassGuitar ? 0 : 1;

    if (instrumentLastFrame == Global::currentInstrument)
      toneAssignmentNames[h][toneAssignmentLastFrame] = Global::vstToneName;
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
    strcpy(Global::vstToneName, toneAssignmentNames[h][Global::toneAssignment].c_str());
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32
    Global::vstToneNameLength = i32(toneAssignmentNames[h][Global::toneAssignment].size());
    Global::toneAssignmentTime = Global::time;
    for (i32 i = 0; i < NUM(Global::effectChain); ++i)
    {
      Global::effectChain[i] = toneAssignmentIndex[h][Global::toneAssignment][i];
      if (Global::effectChain[i] != -1)
      {
        i32 instance = 0;
        for (i32 j = 0; j < i; ++j)
          if (Global::effectChain[i] == Global::effectChain[j])
            ++instance;

        Plugin::loadParameter(Global::effectChain[i], instance, toneAssignmentBase64[h][Global::toneAssignment][i]);
      }
    }
  }

  instrumentLastFrame = Global::currentInstrument;
  toneAssignmentLastFrame = Global::toneAssignment;
#endif // SUPPORT_PLUGIN
}

static void saveStatsOnly()
{
  std::map<std::string, std::map<std::string, std::string>> serializedSaves;

#ifdef SUPPORT_PLUGIN
  std::map<std::string, std::string> toneAssignmentTime;
  for (i32 h = 0; h < 2; ++h)
  {
    const char instChar = h == 0 ? 'B' : 'G';

    for (i32 i = 0; i < Const::profileToneAssignmentCount; ++i)
    {
      if (!toneAssignmentNames[h][i].empty())
        toneAssignmentTime.insert({ std::to_string(i) + instChar, toneAssignmentNames[h][i] });

      for (i32 j = 0; j < NUM(Global::effectChain); ++j)
      {
        if (toneAssignmentIndex[h][i][j] < 0)
          continue;

        const std::string base64 = toneAssignmentBase64[h][i][j];
        toneAssignmentTime.insert({ std::to_string(i) + instChar + std::to_string(j), Global::pluginNames[toneAssignmentIndex[h][i][j]] + ';' + base64 });
      }
    }
  }
  serializedSaves.insert({ "!ToneAssignment", toneAssignmentTime });
#endif // SUPPORT_PLUGIN

  for (const Song::Info& songInfo : Global::songInfos)
  {
    const std::map<std::string, std::map<std::string, std::string>> map = serialize(songInfo.manifestInfos);
    serializedSaves.insert(map.begin(), map.end());
  }

  char profileIni[sizeof("profile_") + NUM(Global::profileName) + sizeof(".ini")] = "profile_";

#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
  strcat(profileIni, Global::profileName);
  strcat(profileIni, ".ini");
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32

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

#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
  strcat(profileIni, Global::profileName);
  strcat(profileIni, ".ini");
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32

  File::saveIni(profileIni, serializedSaves);
}

void Profile::fini()
{
  switch (Global::settings.profileSaveMode)
  {
  case SaveMode::statsOnly:
    saveStatsOnly();
    break;
  case SaveMode::wholeManifest:
    saveWholeManifest();
    break;
  }
}

#ifdef SUPPORT_PLUGIN
void Profile::saveTone()
{
  const i32 h = Global::currentInstrument == InstrumentFlags::BassGuitar ? 0 : 1;

  toneAssignmentNames[h][Global::toneAssignment] = std::string(Global::vstToneName);
  std::vector<i32> instances(Global::pluginNames.size());
  for (i32 i = 0; i < NUM(Global::effectChain); ++i)
  {
    toneAssignmentIndex[h][Global::toneAssignment][i] = Global::effectChain[i];
    if (Global::effectChain[i] >= 0)
    {
      i32 instance = 0;
      for (i32 j = 0; j < i; ++j)
        if (Global::effectChain[i] == Global::effectChain[j])
          ++instance;

      toneAssignmentBase64[h][Global::toneAssignment][i] = Plugin::saveParameters(Global::effectChain[i], instance);
    }
  }
}
#endif // SUPPORT_PLUGIN
