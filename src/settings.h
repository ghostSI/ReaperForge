#ifndef SETTINGS_H
#define SETTINGS_H

#include "helper.h"

namespace Settings {

  struct Info
  {
    i32 audioSampleRate = 48000;
    i32 audioBufferSize = 1024;
    f32 graphicsFieldOfView = 75.0f;
    FullscreenMode graphicsFullscreen = FullscreenMode::windowed;
    u32 graphicsResolutionWidth = 1024;
    u32 graphicsResolutionHeight = 768;
    bool highwayEbeat = true;
    vec4 highwayEbeatColor[2] = {
      colorVec4("#A0AEEFCC"),
      colorVec4("#8795E566")
    };
    bool highwayFretNoteNames = true;
    bool highwayLyrics = true;
    vec4 highwayLyricsColor[3] = {
      colorVec4("#666666"),
      colorVec4("#2381E9"),
      colorVec4("#CCCCCC")
    };
    bool highwaySongInfo = true;
    f32 highwaySpeedMultiplier = 20.0f;
    bool highwayStringNoteNames = true;
    i32 instrumentBassFirstWoundString;
    vec4 instrumentBassStringColor[5] = {
      colorVec4("#E05A01"),
      colorVec4("#2381E9"),
      colorVec4("#D2A20D"),
      colorVec4("#D20000"),
      colorVec4("#009B71"),
    };
    i32 instrumentGuitarFirstWoundString;
    vec4 instrumentGuitarStringColor[7] = {
      colorVec4("#940FB0"),
      colorVec4("#1F9601"),
      colorVec4("#E05A01"),
      colorVec4("#2381E9"),
      colorVec4("#D2A20D"),
      colorVec4("#D20000"),
      colorVec4("#009B71"),
    };
    std::string libraryPath = "songs";
    i32 mixerMusicVolume = 100;
    i32 mixerGuitar1Volume = 100;
    i32 mixerBass1Volume = 100;
    i32 mixerGuitar2Volume = 100;
    i32 mixerBass2Volume = 100;
    i32 mixerMicrophoneVolume = 100;
  };

  bool init(int argc, char* argv[]);

  void fini();
}

#endif // SETTINGS_H
