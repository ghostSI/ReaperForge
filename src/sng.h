#ifndef SNG_H
#define SNG_H

#include "typedefs.h"
#include <vector>

namespace Sng {
  struct Info {
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
