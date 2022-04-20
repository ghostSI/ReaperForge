#ifndef SNG_H
#define SNG_H

#include "typedefs.h"
#include <vector>

namespace Sng {
  struct Info {
    struct Bpm
    {
      f32 time;
      i16 measure;
      i16 beat;
      i32 phraseIteration;
      i32 mask;
    };
    std::vector<Bpm> bpm;

    struct Phrase
    {
      u8 solo;
      u8 disparity;
      u8 ignore;
      u8 paddin;
      i32 maxDifficulty;
      i32 phraseIterationLinks;
      u8 name_[32];
      std::string name;
    };
    std::vector<Phrase> phrase;

    std::string Bpm;
    std::string Phrase;
    std::string Chord;
    std::string ChordNotes;
    std::string Vocal;
    std::string SymbolsHeader;
    std::string SymbolsTexture;
    std::string SymbolDefinition;
    std::string PhraseIteration;
    std::string PhraseExtraInfoByLevel;
    std::string NLinkedDifficulty;
    std::string Action;
    std::string Event;
    std::string Tone;
    std::string Dna;
    std::string Section;
    std::string Arrangement;
    std::string Metadata;
  };

  Sng::Info parse(const std::vector<u8>& sngData);
}

#endif // SNG_H
