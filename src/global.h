#ifndef GLOBAL_H
#define GLOBAL_H

#include "type.h"
#include "helper.h"
#include "song.h"
#include "psarc.h"

#include <vector>
#include <string>

enum struct DisplayMode : u8;
enum struct InputDebugMode : u16;
enum struct RenderOptions : u32;
struct SDL_Window;
typedef struct _SDL_GameController SDL_GameController;

namespace Const {
    inline constexpr i32 randomIntMax = 65535;
    inline constexpr i16 controllerAxisDeadZone = 3000;
    inline constexpr i16 controllerTriggerDeadZone = -30000;
    inline constexpr f32 chordDetectorVolumeThreshhold = 0.005f;
    inline constexpr i32 audioSampleRate = 44100;
    inline constexpr i32 audioBufferSize = 1024;
    inline constexpr f32 highwayRenderMaxFutureTime = -10.0f;
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
    extern KeyInput inputFullscreen;
    extern KeyInput inputPause;
    extern KeyInput inputWireframe;
    extern KeyInput inputDebug;

    extern u32 windowWidth;
    extern u32 windowHeight;
    extern DisplayMode displayMode;
    extern bool pauseAudio;

    extern f32 cameraMidX;
    extern f32 cameraMidY;
    extern mat4 cameraMat;

    extern bool isInstalled;
    extern Instrument filterInstrument;
    extern bool collectionLoaded;
    extern std::vector<Psarc::PsarcInfo> psarcInfos;
    extern std::vector<Song::Info> collection;
    extern std::vector<u8> ogg;
    extern f32 oggStartTime;
    extern char searchText[256];
    extern i32 searchTextLength;

    extern std::atomic<f32> instrumentVolume;
    extern std::atomic<Chords::Note> chordDetectorRootNote;
    extern std::atomic<Chords::Quality> chordDetectorQuality;
    extern std::atomic<i32> chordDetectorIntervals;

    extern f32 frameDelta;
    extern f32 time;

    extern char playerName[256];

    extern bool inputUseController;
    extern i32 inputCursorPosX;
    extern i32 inputCursorPosY;

    extern RenderOptions renderOptions;

    extern GLuint vao; // default vao
    extern GLuint vbo; // default vbo
    extern GLuint fbo; // default fbo is 0 if postprocessor is off
    extern GLuint fboRtt; // framebuffer for rendering to texture (make sure to set framebuffer to 0 after using it)
};

#endif // GLOBAL_H
