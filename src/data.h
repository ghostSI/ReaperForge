#ifndef DATA_H
#define DATA_H

#include "typedefs.h"
#include <vector>

namespace Data {
  namespace Shader
  {
    extern const char* defaultScreenVert;
    extern const char* defaultWorldVert;
    extern const char* defaultFrag;
    extern const char* groundFrag;
    extern const char* fontFrag;
    extern const char* anchorFrag;
    extern const char* ebeatFrag;
    extern const char* chordBoxFrag;
    extern const char* consecutiveChordBoxFrag;
    extern const char* chordBoxArpeggioFrag;
    extern const char* phrasesScreenFrag;
    extern const char* handShapeAnchorFrag;
    extern const char* fretGoldFrag;
    extern const char* fretSilverFrag;
    extern const char* fretBronzeFrag;
    extern const char* stringFrag;
    extern const char* noteStandFrag;
    extern const char* noteStandZeroFrag;
    extern const char* uiFrag;
    extern const char* uiVert;
  }
  namespace Gear
  {
    extern const char* pedalNames[84];
    extern const char* ampNames[64];
    extern const char* cabinetNames[451];
    extern const char* rackNames[18];

    struct Knob
    {
      const char* name;
      f32 defaultValue;
      f32 minValue;
      f32 maxValue;
      f32 valueStep;
    };

    extern std::vector<Gear::Knob> pedalKnobs[83];
    extern std::vector<Gear::Knob> ampKnobs[63];
    extern std::vector<Gear::Knob> rackKnobs[17];
  }
  namespace Texture
  {
    extern const u8 font[82944];
    extern const u8 test[384];
    extern const u8 texture[1048704];
  }
  namespace Geometry
  {
    extern const f32 fret[20820];
    extern const f32 ground[30];
    extern const f32 string[180];


    extern const f32 arpeggio[180];
    extern const f32 arpeggioZeroLeft[120];
    extern const f32 arpeggioZeroRight[120];

    extern const f32 fretMute[540];
    extern const f32 hammerOn[105];
    extern const f32 harmonic[8610];
    extern const f32 harmonicPinch[8610];
    extern const f32 note[150];
    extern const f32 noteFretboard[2460];
    extern const f32 noteStand[180];
    extern const f32 palmMute[690];
    extern const f32 pop[1860];
    extern const f32 pullOff[75];
    extern const f32 slap[1860];
    extern const f32 tap[330];
    extern const f32 zeroLeft[135];
    extern const f32 zeroMiddle[270];
    extern const f32 zeroRight[135];
  }
  namespace Sound
  {
    extern const u8 menuHoverWav[56002];
    extern const u8 menuSelectWav[63634];
  }
}

#endif // DATA_H