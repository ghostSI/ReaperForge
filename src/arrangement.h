#ifndef ARRANGEMENT_H
#define ARRANGEMENT_H

#include "configuration.h"

#ifdef ARRANGEMENT_XML

#include "type.h"
#include <vector>

namespace Arrangement {

  struct Info
  {
    InstrumentFlags instrumentFlags = InstrumentFlags::none;
    std::string title;
    std::string artist;
    bool capo = false;
    std::string albumName;
    i32 albumYear;
    f32 songLength;
    Tuning tuning;

    struct ArrangementProperties
    {
      bool represent;
      bool standardTuning;
      bool nonStandardChords;
      bool barreChords;
      bool powerChords;
      bool dropDPower;
      bool openChords;
      bool fingerPicking;
      bool pickDirection;
      bool doubleStops;
      bool palmMutes;
      bool harmonics;
      bool pinchHarmonics;
      bool hopo;
      bool tremolo;
      bool slides;
      bool unpitchedSlides;
      bool bends;
      bool tapping;
      bool vibrato;
      bool fretHandMutes;
      bool slapPop;
      bool twoFingerPicking;
      bool fifthsAndOctaves;
      bool syncopation;
      bool bassPick;
      bool sustain;
      bool bonusArr;
      bool Metronome;
      bool pathLead;
      bool pathRhythm;
      bool pathBass;
      bool routeMask;
    } arrangementProperties;
  };

  Arrangement::Info readArrangement(const std::vector<u8>& arrangementData);
}

#endif // ARRANGEMENT_XML

#endif // ARRANGEMENT_H
