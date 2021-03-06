#ifndef SETTINGS_H
#define SETTINGS_H

#include "helper.h"

namespace Settings {

  struct Info
  {
    i32 audioBufferSize = 1024;
    i32 audioChannelInstrument[2] = { 0, 1 };
    i32 audioSampleRate = 48000;
    SignalChain audioSignalChain = SignalChain::soundBank;
    f32 cameraBreakRadius = 2.0f;
    f32 cameraFieldOfView = 60.0f;
    f32 cameraMaximumBreakForce = 2.0f;
    f32 cameraMaximumForce = 0.1f;
    f32 cameraMaximumVelocity = 1.5f;
    f32 cameraXFactor = 0.9f;
    f32 cameraXOffset = 1.9f;
    f32 cameraXRotation = 0.247f;
    f32 cameraYFactor = 0.877f;
    f32 cameraYOffset = 3.725f;
    f32 cameraYRotation = 0.175f;
    f32 cameraZFactor = 0.9f;
    f32 cameraZOffset = 7.0f;
    FullscreenMode graphicsFullscreen = FullscreenMode::windowed;
    i32 graphicsWindowWidth = 1024;
    i32 graphicsWindowHeight = 768;
    vec4 highwayAnchorColor[2] = {
      colorVec4("#2E1168"),
      colorVec4("#132693")
    };
    vec4 highwayBackgroundColor = colorVec4("#060A1B");
    vec4 highwayChordBoxColor[2] = {
      colorVec4("#5B74E9"),
      colorVec4("#8795E5")
    };
    vec4 highwayChordNameColor = colorVec4("#ADACAC");
    vec4 highwayDetectorColor = colorVec4("#D81212");
    vec4 highwayDotInlayColor[2] = {
      colorVec4("#EB8D17"),
      colorVec4("#F8B922")
    };
    bool highwayEbeat = true;
    vec4 highwayEbeatColor[2] = {
      colorVec4("#8795E5"),
      colorVec4("#423E6D")
    };
    vec4 highwayFingerNumberColor = colorVec4("#FFFFFF");
    bool highwayFretNoteNames = false;
    vec4 highwayFretNumberColor[3] = {
      colorVec4("#FFBA17CC"),
      colorVec4("#8795E5CC"),
      colorVec4("#4A609ACC")
    };
    vec4 highwayFretboardNoteNameColor[2] = {
      colorVec4("#BCBED4"),
      colorVec4("#8795E5")
    };
    vec4 highwayGroundFretColor[2] = {
      colorVec4("#2037A5"),
      colorVec4("#8795E5")
    };
    bool highwayLyrics = true;
    vec4 highwayLyricsColor[3] = {
      colorVec4("#666666"),
      colorVec4("#1263BC"),
      colorVec4("#CCCCCC")
    };
    vec4 highwayPhraseColor[4] = {
      colorVec4("#7103AF"),
      colorVec4("#2C0337"),
      colorVec4("#D87E34"),
      colorVec4("#444444")
    };
    bool highwayReverseStrings = false;
    bool highwaySongInfo = true;
    vec4 highwaySongInfoColor = colorVec4("#FFFFFF");
    f32 highwaySpeedMultiplier = 20.0f;
    bool highwayStringNoteNames = true;
    bool highwayToneAssignment = true;
    vec4 highwayToneAssignmentColor = colorVec4("#FFFFFF");
    i32 instrumentBass5StringTuning[4] = { -2, -2, -2, -2 };
    i32 instrumentBassFirstWoundString = 0;
    vec4 instrumentBassStringColor[5] = {
      colorVec4("#E05A01"),
      colorVec4("#2381E9"),
      colorVec4("#D2A20D"),
      colorVec4("#D20000"),
      colorVec4("#009B71"),
    };
    i32 instrumentGuitar7StringTuning[6] = { -2, -2, -2, -2, -2, -2 };;
    i32 instrumentGuitarFirstWoundString = 3;
    vec4 instrumentGuitarStringColor[7] = {
      colorVec4("#940FB0"),
      colorVec4("#1F9601"),
      colorVec4("#E05A01"),
      colorVec4("#2381E9"),
      colorVec4("#D2A20D"),
      colorVec4("#D20000"),
      colorVec4("#009B71"),
    };
    u8 midiBinding[128] = { ARR_SET128(0xFF) };
    std::string autoConnectDevices;
    std::string bnkPath = "bnk";
    std::string psarcPath = "psarc";
    std::string vstPath = "vst";
    std::string vst3Path = "vst3";
    i32 mixerMusicVolume = 100;
    i32 mixerGuitar1Volume = 100;
    i32 mixerBass1Volume = 100;
    i32 mixerGuitar2Volume = 100;
    i32 mixerBass2Volume = 100;
    i32 mixerMicrophoneVolume = 100;
    SongFormat profilePreferedSongFormat = SongFormat::xml;
    SaveMode profileSaveMode = SaveMode::statsOnly;
    f32 uiScale = 1.0f;
  };

  bool init(int argc, char* argv[]);

  void fini();
}

#endif // SETTINGS_H
