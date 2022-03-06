#include "global.h"

#include "type.h"

SDL_Window *Global::window = nullptr;
SDL_GameController *Global::gameController = nullptr;
bool Global::appQuit = false;
KeyInput Global::inputFullscreen;
KeyInput Global::inputPause;

u32 Global::windowWidth = 1024;
u32 Global::windowHeight = 768;
DisplayMode Global::displayMode = DisplayMode::windowed;
bool Global::pauseAudio = false;

f32 Global::cameraMidX = f32(Global::windowWidth / 2);
f32 Global::cameraMidY = f32(Global::windowHeight / 2);

Instrument Global::filterInstrument;
bool Global::collectionLoaded;
std::vector<Song::Info> Global::collection;

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
