#ifndef GLOBAL_H
#define GLOBAL_H

#include "type.h"
#include "helper.h"
#include "song.h"
#include "psarc.h"
#include "settings.h"

#include <atomic>
#include <vector>
#include <string>

enum struct FullscreenMode : u8;
enum struct InputDebugMode : u16;
enum struct RenderOptions : u32;
struct SDL_Window;
typedef struct _SDL_GameController SDL_GameController;

namespace Const {

    inline constexpr i32 glMaxVertexMemory = 512 * 1024;
    inline constexpr i32 glMaxElementMemory = 128 * 1024;
    inline constexpr i32 audioMaximumPossibleBufferSize = 2048;
    inline constexpr i32 randomIntMax = 65535;
    inline constexpr i16 controllerAxisDeadZone = 3000;
    inline constexpr i16 controllerTriggerDeadZone = -30000;
    inline constexpr f32 chordDetectorVolumeThreshhold = 0.005f;
    inline constexpr f32 aspectRatio = (16.0f / 9.0f);
    inline constexpr i32 fontCharWidth = 12;
    inline constexpr i32 fontCharHeight = 18;
    inline constexpr i32 fontTextureWidth = 192;
    inline constexpr i32 fontTextureHeight = 108;
    inline constexpr vec3 cameraInitialPosition = {
      .v0 = -6.3f,
      .v1 = -7.0f,
      .v2 = -9.16f
    };
    inline constexpr f32 highwayMaxFutureTime = -10.0f;
    inline constexpr f32 highwayNoteDetectionTimeOffset = 0.4f;
    inline constexpr f32 highwayTremoloFrequency = 0.04f;
    inline constexpr f32 highwayLeftHandPreTime = -2.0f;
    inline constexpr f32 highwayDrawSongInfoStartTime = 1.0f;
    inline constexpr f32 highwayDrawSongInfoFadeInTime = 1.5f;
    inline constexpr f32 highwayDrawSongInfoFadeOutTime = 3.5f;
    inline constexpr f32 highwayDrawSongInfoEndTime = 4.0f;
    inline constexpr f32 highwayDrawToneAssignmentFadeInTime = 0.5f;
    inline constexpr f32 highwayDrawToneAssignmentFadeOutTime = 2.5f;
    inline constexpr f32 highwayDrawToneAssignmentEndTime = 3.0f;
    inline constexpr f32 highwayDrawChordNameWaitTime = 0.2f;
    inline constexpr f32 highwayDrawChordNameFadeOutTime = 0.2f;
    inline constexpr f32 highwayDrawChordNameEndTime = 0.3f;
    inline constexpr f32 highwayStringSpacing = 0.5f;
    inline constexpr f32 highwayStringGaugeMultiplier = 0.2f;
    inline constexpr f32 highwayFretPosition[] =
    {
      0.0f,
      1.0f,
      2.0f,
      3.0f,
      4.0f,
      5.0f,
      6.0f,
      7.0f,
      8.0f,
      9.0f,
      10.0f,
      11.0f,
      12.0f,
      13.0f,
      14.0f,
      15.0f,
      16.0f,
      17.0f,
      18.0f,
      19.0f,
      20.0f,
      21.0f,
      22.0f,
      23.0f,
      24.0f
    };
    inline constexpr i32 instrumentBassStringCount = 5;
    inline constexpr i32 instrumentGuitarStringCount = 7;
    inline constexpr const char notesFlat[][3] =
    {
      "A",
      "B\b",
      "B",
      "C",
      "D\b",
      "D",
      "E\b",
      "E",
      "F",
      "G\b",
      "G",
      "A\b"
    };
    inline constexpr const char notesSharp[][3] =
    {
      "A",
      "A#",
      "B",
      "C",
      "C#",
      "D",
      "D#",
      "E",
      "F",
      "F#",
      "G",
      "G#"
    };
    inline constexpr i32 stringStandardTuningOffset[9] // Standard Tuning
    {
      7,  // e
      2,  // b
      10, // g
      5,  // D
      0,  // A
      7,  // E
      2,  // B
      9,  // F#
      4,  // C#
    };
    inline constexpr bool isMarkedFret[]{ 0,1,0,1,0,1,0,1,0,1,0,0,1,0,0,1,0,1,0,1,0,1,0,0,1 };
    inline constexpr i32 soundMaxCount = 8;
    inline constexpr i32 gearMaxKnobs = 10;
    inline constexpr i32 profileToneAssignmentCount = 10;
}

namespace Global {
    extern SDL_Window *window;
    extern SDL_GameController *gameController;
    extern bool appQuit;
    extern KeyInput inputA;
    extern KeyInput inputD;
    extern KeyInput inputW;
    extern KeyInput inputS;
    extern KeyInput inputE;
    extern KeyInput inputC;
    extern KeyInput inputKP0;
    extern KeyInput inputKP1;
    extern KeyInput inputKP2;
    extern KeyInput inputKP3;
    extern KeyInput inputKP4;
    extern KeyInput inputKP5;
    extern KeyInput inputKP6;
    extern KeyInput inputKP7;
    extern KeyInput inputKP8;
    extern KeyInput inputKP9;
    extern KeyInput inputFullscreen;
    extern KeyInput inputPause;
    extern KeyInput inputWireframe;
    extern KeyInput inputDebug;
    extern KeyInput inputEsc;
    extern KeyInput debugCamera;
    extern KeyInput quickRepeater;

    extern bool pauseAudio;

    extern f32 cameraMidX;
    extern f32 cameraMidY;
    extern mat4 cameraMat;

    extern bool isInstalled;
    extern InstrumentFlags currentInstrument;
    extern i32 bassTuning[5];
    extern i32 guitarTuning[7];
    extern std::vector<Psarc::Info> psarcInfos;
    extern std::vector<Song::Info> songInfos;
    extern i32 songSelected;
    extern i32 manifestSelected;
    extern Song::Track songTrack;
    extern std::vector<Song::Vocal> songVocals;
    extern u8* musicBuffer;
    extern u32 musicBufferLength;
    extern u8* musicBufferPosition;
    extern u32 musicBufferRemainingLength;
    extern f32 musicTimeElapsed;
    extern f32 musicSpeedMultiplier;
    extern char searchText[256];
    extern i32 searchTextLength;
    extern bool toneWindow;
    extern bool helpWindow;
    extern f32 toneAssignment;
#ifdef SUPPORT_VST
    extern bool effectsWindow;
    extern i32 vstWindow;
    extern std::vector<std::string> vstPluginNames;
    extern i32 effectChain[16];
    extern i32 vstToneAssignment;
    extern char vstToneName[256];
    extern i32 vstToneNameLength;
#endif // SUPPORT_VST
    
    extern Settings::Info settings;

    extern std::atomic<f32> instrumentVolume;
    extern std::atomic<Chords::Note> chordDetectorRootNote;
    extern std::atomic<Chords::Quality> chordDetectorQuality;
    extern std::atomic<i32> chordDetectorIntervals;

    extern f32 frameDelta;
    extern f32 time;

    extern char profileName[256];

    extern bool inputUseController;
    extern i32 inputCursorPosX;
    extern i32 inputCursorPosY;

    extern std::atomic<u64> debugAudioCallbackRecording;
    extern std::atomic<u64> debugAudioCallbackPlayback;

    extern GLuint vao; // default vao
    extern GLuint vbo; // default vbo
    extern GLuint ebo;
    extern GLuint fbo; // default fbo is 0 if postprocessor is off
    extern GLuint fboRtt; // framebuffer for rendering to texture (make sure to set framebuffer to 0 after using it)
    extern GLuint texture;
};

#endif // GLOBAL_H
