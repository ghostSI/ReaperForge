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

u32 Global::windowWidth = 1024;
u32 Global::windowHeight = 768;
DisplayMode Global::displayMode = DisplayMode::windowed;
bool Global::pauseAudio = false;

f32 Global::cameraMidX = f32(Global::windowWidth / 2);
f32 Global::cameraMidY = f32(Global::windowHeight / 2);
mat4 Global::cameraMat;

bool Global::isInstalled = false;
InstrumentFlags Global::filterInstrument = InstrumentFlags::none;
bool Global::collectionLoaded = false;
std::vector<Psarc::Info> Global::psarcInfos;
std::vector<Song::Info> Global::collection;
std::vector<u8> Global::ogg;
f32 Global::oggStartTime = 0.0f;
char Global::searchText[256] = "";
i32 Global::searchTextLength = 0;

f32 Global::settingsHighwaySpeedMultiplier = 1.0f;
bool Global::settingsStringNoteNames = false;
bool Global::settingsFretNoteNames = false;
bool Global::settingsShowLyrics = false;
bool Global::settingsShowSongInfo = false;
i32 Global::settingsInstrumentBassFirstWoundString;
vec4 Global::settingsInstrumentBassStringColor[5];
i32 Global::settingsInstrumentGuitarFirstWoundString;
vec4 Global::settingsInstrumentGuitarStringColor[7];
std::atomic<i32> Global::settingsMixerMusicVolume;
std::atomic<i32> Global::settingsMixerGuitar1Volume;
std::atomic<i32> Global::settingsMixerBass1Volume;
std::atomic<i32> Global::settingsMixerGuitar2Volume;
std::atomic<i32> Global::settingsMixerBass2Volume;
std::atomic<i32> Global::settingsMixerMicrophoneVolume;

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

GLuint Global::vao = 0;
GLuint Global::vbo = 0;
GLuint Global::fbo = 0;
GLuint Global::fboRtt = 0;
GLuint Global::texture = 0;
