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
    inline constexpr f32 aspectRatio = (16.0f / 9.0f);
    inline constexpr i32 fontCharWidth = 12;
    inline constexpr i32 fontCharHeight = 18;
    inline constexpr i32 fontTextureWidth = 192;
    inline constexpr i32 fontTextureHeight = 108;
    inline constexpr f32 highwayRenderMaxFutureTime = -10.0f;
    inline constexpr f32 highwayRenderTremoloFrequency = 0.04f;
    inline constexpr f32 highwayRenderLeftHandPreTime = -2.0f;
    inline constexpr f32 highwayRenderDrawSongInfoStartTime = 1.0f;
    inline constexpr f32 highwayRenderDrawSongInfoFadeInTime = 1.5f;
    inline constexpr f32 highwayRenderDrawSongInfoFadeOutTime = 3.5f;
    inline constexpr f32 highwayRenderDrawSongInfoEndTime = 4.0f;
    inline constexpr f32 highwayRenderDrawChordNameWaitTime = 0.2f;
    inline constexpr f32 highwayRenderDrawChordNameFadeOutTime = 0.2f;
    inline constexpr f32 highwayRenderDrawChordNameEndTime = 0.3f;
    inline constexpr f32 highwayRenderStringSpacing = 0.5f;
    inline constexpr f32 highwayRenderStringGaugeMultiplier = 0.2f;
    inline constexpr f32 highwayRenderFretPosition[] =
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
    extern KeyInput debugCamera;

    extern u32 windowWidth;
    extern u32 windowHeight;
    extern DisplayMode displayMode;
    extern bool pauseAudio;

    extern f32 cameraMidX;
    extern f32 cameraMidY;
    extern mat4 cameraMat;

    extern bool isInstalled;
    extern InstrumentFlags filterInstrument;
    extern bool collectionLoaded;
    extern std::vector<Psarc::Info> psarcInfos;
    extern std::vector<Song::Info> collection;
    extern Song::Info songInfo;
    extern Song::Track songTrack;
    extern std::vector<Song::Vocal> songVocals;
    extern std::vector<u8> ogg;
    extern f32 oggStartTime;
    extern char searchText[256];
    extern i32 searchTextLength;

    extern f32 settingsHighwaySpeedMultiplier;
    extern bool settingsStringNoteNames;
    extern bool settingsFretNoteNames;
    extern bool settingsShowLyrics;
    extern bool settingsShowSongInfo;
    extern i32 settingsInstrumentBassFirstWoundString;
    extern vec4 settingsInstrumentBassStringColor[5];
    extern i32 settingsInstrumentGuitarFirstWoundString;
    extern vec4 settingsInstrumentGuitarStringColor[7];
    extern std::atomic<i32> settingsMixerMusicVolume;
    extern std::atomic<i32> settingsMixerGuitar1Volume;
    extern std::atomic<i32> settingsMixerBass1Volume;
    extern std::atomic<i32> settingsMixerGuitar2Volume;
    extern std::atomic<i32> settingsMixerBass2Volume;
    extern std::atomic<i32> settingsMixerMicrophoneVolume;

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
    extern std::atomic<u64> debugAudioCallbackRecording;
    extern std::atomic<u64> debugAudioCallbackPlayback;

    extern GLuint vao; // default vao
    extern GLuint vbo; // default vbo
    extern GLuint fbo; // default fbo is 0 if postprocessor is off
    extern GLuint fboRtt; // framebuffer for rendering to texture (make sure to set framebuffer to 0 after using it)
    extern GLuint texture;
};

#endif // GLOBAL_H
