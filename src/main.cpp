#include "configuration.h"

#ifndef TEST_BUILD

#include "camera.h"
#include "data.h"
#include "debug.h"
#include "font.h"
#include "global.h"
#include "highway.h"
#include "input.h"
#include "installer.h"
#include "opengl.h"
#include "settings.h"
#include "shader.h"
#include "sound.h"
#include "ui.h"

#include "SDL2/SDL.h"

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
    Sound::tick();
    Camera::tick();
#ifndef __EMSCRIPTEN__
    Ui::tick();
#endif // __EMSCRIPTEN__

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifndef __EMSCRIPTEN__
    if (Global::inputWireframe.toggle)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif // __EMSCRIPTEN__

    //Debug::render();
    Highway::render();
    //Camera::render();
#ifndef __EMSCRIPTEN__
    Ui::render();
#endif // __EMSCRIPTEN__

    SDL_GL_SwapWindow(Global::window);

    last_frame = current_frame;
  }
}

int main(int argc, char* argv[]) {
  Installer::init();
  if (!Settings::init(argc, argv))
    return -1;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER)) {
    SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
    SDL_Quit();
  }

  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#if defined __EMSCRIPTEN__ || defined FORCE_OPENGL_ES
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#if not __EMSCRIPTEN__ || _WIN32
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif // __EMSCRIPTEN__
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif // __EMSCRIPTEN__
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  Global::window = SDL_CreateWindow("ReaperForge", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    Global::settings.graphicsResolutionWidth, Global::settings.graphicsResolutionHeight,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

  if (Global::window == nullptr) {
    SDL_Log("Unable to create Window: %s", SDL_GetError());
    SDL_Quit();
  }

  SDL_GLContext con = SDL_GL_CreateContext(Global::window);
  assert(con);

#ifndef __EMSCRIPTEN__
  SDL_GL_SetSwapInterval(0); // disable vsync
#endif // __EMSCRIPTEN__

  //glEnable(GL_CULL_FACE);
  //glCullFace(GL_FRONT);
  //glFrontFace(GL_CCW);
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

  if (Global::gameController == nullptr && SDL_NumJoysticks() >= 1 && SDL_IsGameController(0))
    Global::gameController = SDL_GameControllerOpen(0);

  glClearColor(0.0f, 0.0031372f, 0.0721568f, 1.0f);

  Shader::init();
  Sound::init();
  Camera::init();
  Highway::init();
  Font::init();
#ifndef __EMSCRIPTEN__
  Ui::init();
#endif // __EMSCRIPTEN__

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(mainloop, -1, 1);
#else
  while (!Global::appQuit)
    mainloop();
#endif // __EMSCRIPTEN__

  Settings::fini();

  SDL_Quit();
  return 0;
}

#endif // TEST_BUILD
