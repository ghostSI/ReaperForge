#include "configuration.h"

#include "bnk.h"
#include "camera.h"
#include "collection.h"
#include "data.h"
#include "debug.h"
#include "font.h"
#include "global.h"
#include "highway.h"
#include "input.h"
#include "installer.h"
#include "midi.h"
#include "opengl.h"
#include "phrases.h"
#include "player.h"
#include "plugin.h"
#include "profile.h"
#include "settings.h"
#include "shader.h"
#include "sound.h"
#include "test.h"
#include "ui.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <chrono>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif // __EMSCRIPTEN__

static void mainloop() {
  static std::chrono::high_resolution_clock::time_point last_frame = std::chrono::high_resolution_clock::now();
  static std::chrono::high_resolution_clock::time_point current_frame;

  current_frame = std::chrono::high_resolution_clock::now();

  Global::frameDelta = std::chrono::duration<f32, std::milli>(current_frame - last_frame).count();
  Global::time += 0.001_f32 * Global::frameDelta;

  {
    Input::prePollEvent();
#ifndef __EMSCRIPTEN__
    Ui::handleInputBegin();
#endif // __EMSCRIPTEN__
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      Input::pollEvent(event);
#ifndef __EMSCRIPTEN__
      Ui::handleInput(event);
#endif // __EMSCRIPTEN__
    }
#ifndef __EMSCRIPTEN__
    Ui::handleInputEnd();
#endif // __EMSCRIPTEN__
    Input::postPollEvent();

    Input::proccessInputEvents();
  }

  //if (Global::frameDelta >= 16.666_f32)
  { // render frame
#ifdef SUPPORT_BNK
    Bnk::tick();
#endif // SUPPORT_BNK
    Profile::tick();
    Player::tick();
    Phrases::tick();
    Highway::tick();
    Camera::tick();
#ifndef __EMSCRIPTEN__
    if (!Global::inputEsc.toggle)
      Ui::tick();
#endif // __EMSCRIPTEN__

    glClearColor(Global::settings.highwayBackgroundColor.v0, Global::settings.highwayBackgroundColor.v1, Global::settings.highwayBackgroundColor.v2, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifndef __EMSCRIPTEN__
    if (Global::inputWireframe.toggle)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif // __EMSCRIPTEN__

    Phrases::render();
    Highway::render();
    Camera::render();
#ifndef __EMSCRIPTEN__
    if (!Global::inputEsc.toggle)
      Ui::render();
#endif // __EMSCRIPTEN__

    Debug::render();

    SDL_GL_SwapWindow(Global::window);

    last_frame = current_frame;
  }
}

int main(int argc, char* argv[]) {
#ifdef RUN_TEST
  Test::run();
#endif // RUN_TEST

  Installer::init();
  if (!Settings::init(argc, argv))
    return -1;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER)) {
    SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
    SDL_Quit();
  }

#if defined __EMSCRIPTEN__ || defined FORCE_OPENGL_ES
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#ifdef _WIN32
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif // _WIN32
#else // __EMSCRIPTEN__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif // __EMSCRIPTEN__
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  i32 fullScreenFlag;
  switch (Global::settings.graphicsFullscreen)
  {
  case FullscreenMode::windowed:
    fullScreenFlag = 0;
    Global::resolutionWidth = Global::settings.graphicsWindowWidth;
    Global::resolutionHeight = Global::settings.graphicsWindowHeight;
    break;
  case FullscreenMode::borderless:
    fullScreenFlag = SDL_WINDOW_FULLSCREEN_DESKTOP;
    break;
  default:
    assert(false);
    break;
  }

  Global::window = SDL_CreateWindow("ReaperForge", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    Global::settings.graphicsWindowWidth, Global::settings.graphicsWindowHeight,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | fullScreenFlag);

  if (Global::window == nullptr) {
    SDL_Log("Unable to create Window: %s", SDL_GetError());
    SDL_Quit();
  }

  {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(Global::window, &wmInfo);
    Global::hWnd = wmInfo.info.win.window;
  }

  SDL_GLContext con = SDL_GL_CreateContext(Global::window);
  assert(con);

#ifndef __EMSCRIPTEN__
  SDL_GL_SetSwapInterval(0); // disable vsync
#endif // __EMSCRIPTEN__

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_SCISSOR_TEST);

  OpenGl::init();
  glGenVertexArrays(1, &Global::vao);
  glGenBuffers(1, &Global::vbo);
  glGenBuffers(1, &Global::ebo);
  glBindVertexArray(Global::vao);
  glBindBuffer(GL_ARRAY_BUFFER, Global::vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Global::ebo);
  glGenFramebuffers(1, &Global::fboRtt);
  Global::texture = OpenGl::loadDDSTexture(Data::Texture::texture, sizeof(Data::Texture::texture));
  Global::textureError = OpenGl::loadDDSTexture(Data::Texture::textureError, sizeof(Data::Texture::textureError));

  if (Global::gameController == nullptr && SDL_NumJoysticks() >= 1 && SDL_IsGameController(0))
    Global::gameController = SDL_GameControllerOpen(0);

  Shader::init();
  Plugin::init();
  Profile::init();
  Sound::init();
#ifdef SUPPORT_BNK
  Bnk::init();
#endif // SUPPORT_BNK
  Camera::init();
  Font::init();
  Collection::init();
#ifdef SUPPORT_MIDI
  Midi::init();
#endif // SUPPORT_MIDI

#ifndef __EMSCRIPTEN__
  Ui::init();
#endif // __EMSCRIPTEN__


#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(mainloop, -1, 1);
#else
  while (!Global::appQuit)
    mainloop();
#endif // __EMSCRIPTEN__

#ifdef SUPPORT_MIDI
  Midi::fini();
#endif // SUPPORT_MIDI
  Profile::fini();
  Settings::fini();

  quick_exit(0);
}
