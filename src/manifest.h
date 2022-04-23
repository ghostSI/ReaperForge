#ifndef MANIFEST_H
#define MANIFEST_H

#include "type.h"
#include <vector>
#include <string>

namespace Manifest {

  struct Info
  {
    struct Attributes
    {
      std::string albumArt;
      std::string albumName;
      std::string albumNameSort;
      std::string arrangementName;
      std::string artistName;
      std::string artistNameSort;
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
      struct
      {
        i32 string0{};
        i32 string1{};
        i32 string2{};
        i32 string3{};
        i32 string4{};
        i32 string5{};
      } tuning;
      std::string persistentID;
    };

    std::vector<Attributes> attributes;
    std::string insertRoot;
  };

  Manifest::Info readHsan(const std::vector<u8>& hsanData);
}

#endif // MANIFEST_H
