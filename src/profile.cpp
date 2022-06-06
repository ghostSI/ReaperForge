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

#ifdef SUPPORT_VST
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
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
  },
  {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
  }
};
static std::string toneAssignmentBase64[2][Const::profileToneAssignmentCount][NUM(Global::effectChain)];
#endif // SUPPORT_VST

static void loadStatsOnly()
{
  char profileIni[sizeof("profile_") + NUM(Global::profileName) + sizeof(".ini")] = "profile_";
  strcat(profileIni, Global::profileName);
  strcat(profileIni, ".ini");

  if (!File::exists(profileIni))
    return;

  const std::map<std::string, std::map<std::string, std::string>> serializedSaves = File::loadIni(profileIni);

#ifdef SUPPORT_VST
  const auto search = serializedSaves.find("!ToneAssignment");
  if (search != serializedSaves.end())
  {
    const std::map<std::string, std::string>& toneAssignment = search->second;

    for (i32 h = 0; h < 2; ++h)
    {
      const char instChar = h == 0 ? 'B' : 'G';

      for (i32 i = 0; i < Const::profileToneAssignmentCount; ++i)
      {
        if (const auto search3 = toneAssignment.find(std::to_string(i) + instChar); search3 != toneAssignment.end())
        {
          toneAssignmentNames[h][i] = search3->second;
        }

        for (i32 j = 0; j < NUM(Global::effectChain); ++j)
        {
          toneAssignmentIndex[h][i][j] = -1;
          const auto search2 = toneAssignment.find(std::to_string(i) + instChar + std::to_string(j));
          if (search2 != toneAssignment.end())
          {
            const u64 seperator = search2->second.find_last_of(';');
            assert(seperator != std::string::npos);
            const std::string pluginName = search2->second.substr(0, seperator);
            if (!pluginName.empty())
            {
              for (i32 k = 0; k < Global::vstPluginNames.size(); ++k)
              {
                if (pluginName == Global::vstPluginNames[k])
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

  {
    strcpy(Global::vstToneName, toneAssignmentNames[1][0].c_str());
    Global::vstToneNameLength = toneAssignmentNames[1][0].size();
    for (i32 j = 0; j < NUM(Global::effectChain); ++j)
    {
      Global::effectChain[j] = toneAssignmentIndex[1][0][j];
      if (Global::effectChain[j] >= 0)
        Vst::loadParameter(j, toneAssignmentBase64[1][0][j]);
    }
  }
#endif // SUPPORT_VST


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

void Profile::tick()
{
#ifdef SUPPORT_VST
  static InstrumentFlags instrumentLastFrame = Global::currentInstrument;
  static i32 vstToneAssignmentLastFrame = Global::vstToneAssignment;

  if (instrumentLastFrame != Global::currentInstrument || vstToneAssignmentLastFrame != Global::vstToneAssignment)
  {
    const i32 h = Global::currentInstrument == InstrumentFlags::BassGuitar ? 0 : 1;

    if (instrumentLastFrame == Global::currentInstrument)
      toneAssignmentNames[h][vstToneAssignmentLastFrame] = Global::vstToneName;
    strcpy(Global::vstToneName, toneAssignmentNames[h][Global::vstToneAssignment].c_str());
    Global::vstToneNameLength = toneAssignmentNames[h][Global::vstToneAssignment].size();
    Global::toneAssignment = Global::time;
    for (i32 i = 0; i < NUM(Global::effectChain); ++i)
    {
      Global::effectChain[i] = toneAssignmentIndex[h][Global::vstToneAssignment][i];
      if (Global::effectChain[i] != -1)
        Vst::loadParameter(i, toneAssignmentBase64[h][Global::vstToneAssignment][i]);
    }
  }

  instrumentLastFrame = Global::currentInstrument;
  vstToneAssignmentLastFrame = Global::vstToneAssignment;
#endif // SUPPORT_VST
}

static void saveStatsOnly()
{
  std::map<std::string, std::map<std::string, std::string>> serializedSaves;

#ifdef SUPPORT_VST
  std::map<std::string, std::string> toneAssignment;
  for (i32 h = 0; h < 2; ++h)
  {
    const char instChar = h == 0 ? 'B' : 'G';

    for (i32 i = 0; i < Const::profileToneAssignmentCount; ++i)
    {
      if (!toneAssignmentNames[h][i].empty())
        toneAssignment.insert({ std::to_string(i) + instChar, toneAssignmentNames[h][i] });

      for (i32 j = 0; j < NUM(Global::effectChain); ++j)
      {
        if (toneAssignmentIndex[h][i][j] < 0)
          continue;

        const std::string base64 = toneAssignmentBase64[h][i][j];
        toneAssignment.insert({ std::to_string(i) + instChar + std::to_string(j), Global::vstPluginNames[toneAssignmentIndex[h][i][j]] + ';' + base64 });
      }
    }
  }
  serializedSaves.insert({ "!ToneAssignment", toneAssignment });
#endif // SUPPORT_VST

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

#ifdef SUPPORT_VST
void Profile::saveTone()
{
  const i32 h = Global::currentInstrument == InstrumentFlags::BassGuitar ? 0 : 1;

  toneAssignmentNames[h][Global::vstToneAssignment] = std::string(Global::vstToneName);
  for (i32 i = 0; i < NUM(Global::effectChain); ++i)
  {
    toneAssignmentIndex[h][Global::vstToneAssignment][i] = Global::effectChain[i];
    if (Global::effectChain[i] >= 0)
      toneAssignmentBase64[h][Global::vstToneAssignment][i] = Vst::saveParameters(Global::effectChain[i]);
  }
}
#endif // SUPPORT_VST
