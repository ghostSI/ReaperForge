#ifndef XBLOCK_H
#define XBLOCK_H

#include "type.h"
#include <vector>

namespace XBlock {
  struct Info
  {
    struct Entry
    {
      InstrumentFlags instrumentFlags;
      char id[32];
#ifdef XBLOCK_FULL
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
#endif
    };

    std::vector<Entry> entries;
  };

  XBlock::Info readXBlock(const std::vector<u8>& xBlockData);
}

#endif // XBLOCK_H
