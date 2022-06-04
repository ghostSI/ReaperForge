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
struct VstTone
{
  struct Tone
  {
    std::string name;
    struct Assignemnt
    {
      i32 index = -1;
      std::vector<u8> vstData;
    } assignment;
  };

  Tone bass[NUM(Global::effectChain)];
  Tone guitar[NUM(Global::effectChain)];
};
static VstTone vstTones[Const::profileToneAssignmentCount];
#endif // SUPPORT_VST

static void loadStatsOnly()
{
  char profileIni[sizeof("profile_") + NUM(Global::profileName) + sizeof(".ini")] = "profile_";
  strcat(profileIni, Global::profileName);
  strcat(profileIni, ".ini");

  if (!File::exists(profileIni))
    return;

  const std::map<std::string, std::map<std::string, std::string>> serializedSaves = File::loadIni(profileIni);

  {
    const auto search = serializedSaves.find("!ToneAssignment");
    if (search != serializedSaves.end())
    {
      const std::map<std::string, std::string>& toneAssignment = search->second;

      for (i32 i = 0; i < Const::profileToneAssignmentCount; ++i)
      {
        for (i32 j = 0; j < NUM(Global::effectChain); ++j)
        {
          const auto search2 = toneAssignment.find(std::to_string(i) + "G" + std::to_string(j));
          if (search2 != toneAssignment.end())
          {
            const u64 seperator = search2->second.find_last_of(';');
            assert(seperator != std::string::npos);
            const std::string pluginName = search2->second.substr(0, seperator);
            const std::string base64 = search2->second.substr(seperator + 1);

            for (i32 k = 0; k < Global::vstPluginNames.size(); ++k)
            {
              if (pluginName == Global::vstPluginNames[k])
              {
                Global::effectChain[j] = k;
                Vst::loadParameter(j, base64);
                break;
              }
            }
          }
        }
      }
    }
  }

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



static void saveStatsOnly()
{
  std::map<std::string, std::map<std::string, std::string>> serializedSaves;

  std::map<std::string, std::string> toneAssignment;

  for (i32 h = 0; h < 2; ++h)
  {
    const char instrument = (h == 0 ? 'G' : 'B');

    for (i32 i = 0; i < Const::profileToneAssignmentCount; ++i)
    {
      toneAssignment.insert({ std::to_string(i) + instrument, "Default" });

      for (i32 j = 0; j < NUM(Global::effectChain); ++j)
      {
        if (Global::effectChain[j] < 0)
          continue;

        const std::string parameter = Vst::saveParameters(Global::effectChain[j]);
        toneAssignment.insert({ std::to_string(i) + instrument + std::to_string(j), Global::vstPluginNames[Global::effectChain[j]] + ';' + parameter });
      }
    }
    serializedSaves.insert({ "!ToneAssignment", toneAssignment });
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