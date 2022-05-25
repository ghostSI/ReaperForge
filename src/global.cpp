#include "global.h"

#include "type.h"

SDL_Window *Global::window = nullptr;
SDL_GameController *Global::gameController = nullptr;
bool Global::appQuit = false;
KeyInput Global::inputA;
KeyInput Global::inputD;
KeyInput Global::inputW;
KeyInput Global::inputS;
KeyInput Global::inputE;
KeyInput Global::inputC;
KeyInput Global::inputFullscreen;
KeyInput Global::inputPause;
KeyInput Global::inputWireframe;
KeyInput Global::inputDebug;
KeyInput Global::inputEsc;
KeyInput Global::debugCamera;

bool Global::pauseAudio = false;

f32 Global::cameraMidX = f32(Global::settings.graphicsResolutionWidth / 2);
f32 Global::cameraMidY = f32(Global::settings.graphicsResolutionHeight / 2);
mat4 Global::cameraMat;

bool Global::isInstalled = false;
InstrumentFlags Global::filterInstrument = InstrumentFlags::none;
std::vector<Psarc::Info> Global::psarcInfos;
i32 Global::songSelected = 0;
i32 Global::manifestSelected = 0;
std::vector<Song::Info> Global::songInfos;
Song::Track Global::songTrack;
std::vector<Song::Vocal> Global::songVocals;
std::vector<u8> Global::ogg;
f32 Global::oggStartTime = 0.0f;
char Global::searchText[256] = "";
i32 Global::searchTextLength = 0;
bool Global::toneWindow = false;

Settings::Info Global::settings;

std::atomic<f32> Global::instrumentVolume;
std::atomic<Chords::Note> Global::chordDetectorRootNote;
std::atomic<Chords::Quality> Global::chordDetectorQuality;
std::atomic<i32> Global::chordDetectorIntervals;

f32 Global::frameDelta = 0.016_f32;
f32 Global::time = 0.0_f32;

char Global::playerName[256] = "Anon";

bool Global::inputUseController;
i32 Global::inputCursorPosX = 0;
i32 Global::inputCursorPosY = 0;

RenderOptions Global::renderOptions = RenderOptions::lit;
std::atomic<u64> Global::debugAudioCallbackRecording = 0;
std::atomic<u64> Global::debugAudioCallbackPlayback = 0;

GLuint Global::vao = 0;
GLuint Global::vbo = 0;
GLuint Global::ebo = 0;
GLuint Global::fbo = 0;
GLuint Global::fboRtt = 0;
GLuint Global::texture = 0;
