#include "global.h"

SDL_Window* Global::window = nullptr;
SDL_GameController* Global::gameController = nullptr;
bool Global::appQuit = false;
KeyInput Global::inputA;
KeyInput Global::inputD;
KeyInput Global::inputW;
KeyInput Global::inputS;
KeyInput Global::inputE;
KeyInput Global::inputC;
KeyInput Global::inputKP0;
KeyInput Global::inputKP1;
KeyInput Global::inputKP2;
KeyInput Global::inputKP3;
KeyInput Global::inputKP4;
KeyInput Global::inputKP5;
KeyInput Global::inputKP6;
KeyInput Global::inputKP7;
KeyInput Global::inputKP8;
KeyInput Global::inputKP9;
KeyInput Global::inputFullscreen;
KeyInput Global::inputPause;
KeyInput Global::inputWireframe;
KeyInput Global::inputDebug;
KeyInput Global::inputEsc;
KeyInput Global::debugCamera;
KeyInput Global::quickRepeater;

bool Global::pauseAudio = false;

f32 Global::cameraMidX = f32(Global::settings.graphicsResolutionWidth / 2);
f32 Global::cameraMidY = f32(Global::settings.graphicsResolutionHeight / 2);
mat4 Global::cameraMat;

bool Global::isInstalled = false;
InstrumentFlags Global::currentInstrument = InstrumentFlags::LeadGuitar;
i32 Global::bassTuning[5] = { 0, 0, 0, 0, 0 };
i32 Global::guitarTuning[7] = { 0, 0, 0, 0, 0, 0, 0 };
std::vector<Psarc::Info> Global::psarcInfos;
i32 Global::songSelected = -1;
i32 Global::manifestSelected = 0;
std::vector<Song::Info> Global::songInfos;
Song::Track Global::songTrack;
std::vector<Song::Vocal> Global::songVocals;
u8* Global::musicBuffer = nullptr;
u32 Global::musicBufferLength = 0;
u8* Global::musicBufferPosition = nullptr;
u32 Global::musicBufferRemainingLength = 0;
f32 Global::musicTimeElapsed = 0.0f;
f32 Global::musicSpeedMultiplier = 1.0f;
char Global::searchText[256] = "";
i32 Global::searchTextLength = 0;
bool Global::toneWindow = false;
bool Global::helpWindow = false;
f32 Global::toneAssignment = 0.0f;
#ifdef SUPPORT_VST
bool Global::effectsWindow = false;
i32 Global::vstWindow = -1;
std::vector<std::string> Global::vstPluginNames;
i32 Global::effectChain[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
i32 Global::vstToneAssignment = 0;
char Global::vstToneName[256] = "Default";
i32 Global::vstToneNameLength = sizeof("Default") - 1;
#endif // SUPPORT_VST
#ifdef SUPPORT_MIDI
bool Global::midiWindow = false;
std::vector<std::string> Global::midiDeviceNames;
u8 Global::midiLearnNote = 0xFF;
#endif // SUPPORT_MIDI

Settings::Info Global::settings;

std::atomic<f32> Global::instrumentVolume;
std::atomic<Chords::Note> Global::chordDetectorRootNote;
std::atomic<Chords::Quality> Global::chordDetectorQuality;
std::atomic<i32> Global::chordDetectorIntervals;

f32 Global::frameDelta = 0.016_f32;
f32 Global::time = 0.0_f32;

char Global::profileName[256] = "Anon";

bool Global::inputUseController;
i32 Global::inputCursorPosX = 0;
i32 Global::inputCursorPosY = 0;

std::atomic<u64> Global::debugAudioCallbackRecording = 0;
std::atomic<u64> Global::debugAudioCallbackPlayback = 0;

GLuint Global::vao = 0;
GLuint Global::vbo = 0;
GLuint Global::ebo = 0;
GLuint Global::fbo = 0;
GLuint Global::fboRtt = 0;
GLuint Global::texture = 0;
