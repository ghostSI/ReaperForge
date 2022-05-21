#ifndef MANIFEST_H
#define MANIFEST_H

#include "type.h"
#include <vector>
#include <string>

namespace XBlock { struct Info; }

namespace Manifest {

  struct Info
  {
    struct Entry
    {
      InstrumentFlags instrumentFlags = InstrumentFlags::none;

      std::string albumArt;
      std::string albumName;
      std::string albumNameSort;
      std::string arrangementName;
      std::string artistName;
      std::string artistNameSort;
      i32 bassPick{};
      f32 capoFret{};
      f32 centOffset{};
      bool dLC{};
      std::string dLCKey;
      f32 dNA_Chords{};
      f32 dNA_Riffs{};
      f32 dNA_Solo{};
      f32 easyMastery{};
      i32 leaderboardChallengeRating{};
      std::string manifestUrn;
      i32 masterID_RDV{};
      i32 metronome{};
      f32 mediumMastery{};
      f32 notesEasy{};
      f32 notesHard{};
      f32 notesMedium{};
      i32 representative{};
      i32 routeMask{};
      bool shipping{};
      std::string sKU;
      f32 songDiffEasy{};
      f32 songDiffHard{};
      f32 songDiffMed{};
      f32 songDifficulty{};
      std::string songKey;
      f32 songLength{};
      std::string songName;
      std::string songNameSort;
      i32 songYear{};
      std::string japaneseSongName; // TODO is this in use?
      std::string japaneseArtist; // TODO is this in use?
      Tuning tuning;
      std::string persistentID;

      std::string fileName;
      f32 score{};
      u64 lastPlayed{};
    };

    std::vector<Entry> entries;
  };

  Manifest::Info readHsan(const std::vector<u8>& hsanData, const XBlock::Info& xblock);

  struct Tone
  {
    struct GearList
    {
      struct Gear
      {
        struct KnobValue
        {
          std::string name;
          f32 value{};
        };

        std::string type;
        std::vector<KnobValue> knobValues;
        std::string key;
        std::string category;
      };

      Gear prePedal[4];
      Gear amp;
      Gear postPedal[4];
      Gear cabinet;
      Gear rack[4];
    } gearList;

    bool isCustom{};
    f32 volume{};
    std::vector<std::string> toneDescriptors;
    std::string key;
    std::string nameSeparator;
    std::string name;
    f32 sortOrder{};
  };

  std::vector<Manifest::Tone> readJson(const std::vector<u8>& jsonData);
}

#endif // MANIFEST_H
