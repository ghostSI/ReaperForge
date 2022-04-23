#ifndef XBLOCK_H
#define XBLOCK_H

#include "type.h"
#include <vector>

namespace XBlock {
  struct Info
  {
    struct Entry
    {
      InstrumentFlags instrument;
      char id[32];
      struct Properties
      {
        std::string header;
        std::string manifest;
        std::string sngAsset;
        std::string albumArtSmall;
        std::string albumArtMedium;
        std::string albumArtLarge;
        std::string lyricArt;
        std::string showLightsXMLAsset;
        std::string soundBank;
        std::string previewSoundBank;
      } properties;
    };

    std::vector<Entry> entries;
  };

  XBlock::Info readXBlock(const std::vector<u8>& xBlockData);
}

#endif // XBLOCK_H
