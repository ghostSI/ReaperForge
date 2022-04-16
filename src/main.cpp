#include "configuration.h"

#ifndef TEST_BUILD

#include "global.h"
#include "input.h"
#include "scene.h"
#include "sound.h"
#include "shader.h"
#include "opengl.h"
#include "installer.h"
#include "texture.h"
#include "log.h"
#include "settings.h"
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

    if (Global::frameDelta >= 33.333_f32) {
        Log::infoMsg("Can't keep up. Framedelta %.1f", Global::frameDelta);
        Global::frameDelta = 33.333_f32;
    }

    {
        Input::prePollEvent();
        //Ui::handleInputBegin();
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            Input::pollEvent(event);
            //Ui::handleInput(event);
        }
        //Ui::handleInputEnd();
        Input::postPollEvent();

        Input::proccessInputEvents();
    }

    //if (Global::frameDelta >= 16.666_f32)
    { // render frame
        Sound::tick();
        Scene::tick();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifndef __EMSCRIPTEN__
        if (Global::inputWireframe.toggle)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif // __EMSCRIPTEN__

        Scene::render();

        SDL_GL_SwapWindow(Global::window);

        last_frame = current_frame;
    }
}

int main(int argc, char *argv[]) {
    Installer::init();
    if (!Settings::init(argc, argv))
        return -1;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER)) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        SDL_Quit();
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    Global::window = SDL_CreateWindow("ReaperForge", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                      Global::windowWidth, Global::windowHeight,
                                      SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    if (Global::window == nullptr) {
        SDL_Log("Unable to create Window: %s", SDL_GetError());
        SDL_Quit();
    }

    SDL_GLContext con = SDL_GL_CreateContext(Global::window);
    SDL_GL_SetSwapInterval(0); // disable vsync

    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_FRONT);
    //glFrontFace(GL_CW);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    OpenGl::init();
    OpenGl::glGenVertexArrays(1, &Global::vao);
    OpenGl::glGenBuffers(1, &Global::vbo);
    OpenGl::glBindVertexArray(Global::vao);
    OpenGl::glBindBuffer(GL_ARRAY_BUFFER, Global::vbo);
    OpenGl::glGenFramebuffers(1, &Global::fboRtt);

    if (Global::gameController == nullptr && SDL_NumJoysticks() >= 1 && SDL_IsGameController(0))
      Global::gameController = SDL_GameControllerOpen(0);

    glClearColor(0.0, 0.007843, 0.180392, 1.0);

    Shader::init();
    Sound::init();
    Texture::init();
    Scene::init();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainloop, -1, 1);
#else
    while (!Global::appQuit)
        mainloop();
#endif // __EMSCRIPTEN__

    Settings::save();

    SDL_Quit();
    return 0;
}

#endif // TEST_BUILD
