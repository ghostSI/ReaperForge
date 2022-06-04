#include "input.h"

#include "global.h"
#include "opengl.h"
#include "sound.h"
#include "type.h"

#include "SDL2/SDL_events.h"

static void prePollEventKeyInput(KeyInput& keyInput) {
  keyInput.pressedLastFrame = keyInput.pressed;
  keyInput.down = false;
  keyInput.up = false;
}

void Input::prePollEvent() {
  prePollEventKeyInput(Global::inputA);
  prePollEventKeyInput(Global::inputD);
  prePollEventKeyInput(Global::inputW);
  prePollEventKeyInput(Global::inputS);
  prePollEventKeyInput(Global::inputE);
  prePollEventKeyInput(Global::inputC);
  prePollEventKeyInput(Global::inputKP0);
  prePollEventKeyInput(Global::inputKP1);
  prePollEventKeyInput(Global::inputKP2);
  prePollEventKeyInput(Global::inputKP3);
  prePollEventKeyInput(Global::inputKP4);
  prePollEventKeyInput(Global::inputKP5);
  prePollEventKeyInput(Global::inputKP6);
  prePollEventKeyInput(Global::inputKP7);
  prePollEventKeyInput(Global::inputKP8);
  prePollEventKeyInput(Global::inputKP9);
  prePollEventKeyInput(Global::inputFullscreen);
  prePollEventKeyInput(Global::inputPause);
  prePollEventKeyInput(Global::inputWireframe);
  prePollEventKeyInput(Global::inputDebug);
  prePollEventKeyInput(Global::inputEsc);
  prePollEventKeyInput(Global::debugCamera);
  prePollEventKeyInput(Global::quickRepeater);
}

void Input::pollEvent(SDL_Event& event) {
  switch (event.type) {
  case SDL_QUIT:
    Global::appQuit = true;
    break;
  case SDL_KEYDOWN: {
    switch (event.key.keysym.sym) {
    case SDLK_a:
      Global::inputA.pressed = true;
      break;
    case SDLK_d:
      Global::inputD.pressed = true;
      break;
    case SDLK_w:
      Global::inputW.pressed = true;
      break;
    case SDLK_s:
      Global::inputS.pressed = true;
      break;
    case SDLK_e:
      Global::inputE.pressed = true;
      break;
    case SDLK_c:
      Global::inputC.pressed = true;
      break;
    case SDLK_KP_0:
      Global::inputKP0.pressed = true;
      break;
    case SDLK_KP_1:
      Global::inputKP1.pressed = true;
      break;
    case SDLK_KP_2:
      Global::inputKP2.pressed = true;
      break;
    case SDLK_KP_3:
      Global::inputKP3.pressed = true;
      break;
    case SDLK_KP_4:
      Global::inputKP4.pressed = true;
      break;
    case SDLK_KP_5:
      Global::inputKP5.pressed = true;
      break;
    case SDLK_KP_6:
      Global::inputKP6.pressed = true;
      break;
    case SDLK_KP_7:
      Global::inputKP7.pressed = true;
      break;
    case SDLK_KP_8:
      Global::inputKP8.pressed = true;
      break;
    case SDLK_KP_9:
      Global::inputKP9.pressed = true;
      break;
    case SDLK_LEFT:
      break;
    case SDLK_RIGHT:
      break;
    case SDLK_UP:
      break;
    case SDLK_DOWN:
      break;
    case SDLK_SPACE:
      break;
    case SDLK_TAB:
      break;
    case SDLK_p:
      Global::inputPause.pressed = true;
      break;
    case SDLK_F2:
      Global::inputWireframe.pressed = true;
      break;
    case SDLK_F3:
      Global::inputDebug.pressed = true;
      break;
    case SDLK_F4:
      Global::debugCamera.pressed = true;
      break;
    case SDLK_F5:
      Global::quickRepeater.pressed = true;
      break;
    case SDLK_ESCAPE:
      Global::inputEsc.pressed = true;
      break;
    case SDLK_RETURN:
      if (event.key.keysym.mod & KMOD_ALT)
        Global::inputFullscreen.pressed = true;
      break;
    }
  }
                  break;

  case SDL_KEYUP: {
    switch (event.key.keysym.sym) {
    case SDLK_a:
      Global::inputA.pressed = false;
      break;
    case SDLK_d:
      Global::inputD.pressed = false;
      break;
    case SDLK_w:
      Global::inputW.pressed = false;
      break;
    case SDLK_s:
      Global::inputS.pressed = false;
      break;
    case SDLK_e:
      Global::inputE.pressed = false;
      break;
    case SDLK_c:
      Global::inputC.pressed = false;
      break;
    case SDLK_KP_0:
      Global::inputKP0.pressed = false;
      break;
    case SDLK_KP_1:
      Global::inputKP1.pressed = false;
      break;
    case SDLK_KP_2:
      Global::inputKP2.pressed = false;
      break;
    case SDLK_KP_3:
      Global::inputKP3.pressed = false;
      break;
    case SDLK_KP_4:
      Global::inputKP4.pressed = false;
      break;
    case SDLK_KP_5:
      Global::inputKP5.pressed = false;
      break;
    case SDLK_KP_6:
      Global::inputKP6.pressed = false;
      break;
    case SDLK_KP_7:
      Global::inputKP7.pressed = false;
      break;
    case SDLK_KP_8:
      Global::inputKP8.pressed = false;
      break;
    case SDLK_KP_9:
      Global::inputKP9.pressed = false;
      break;
    case SDLK_LEFT:
      break;
    case SDLK_RIGHT:
      break;
    case SDLK_UP:
      break;
    case SDLK_DOWN:
      break;
    case SDLK_SPACE:
      break;
    case SDLK_TAB:
      break;
    case SDLK_p:
      Global::inputPause.pressed = false;
      break;
    case SDLK_F2:
      Global::inputWireframe.pressed = false;
      break;
    case SDLK_F3:
      Global::inputDebug.pressed = false;
      break;
    case SDLK_F4:
      Global::debugCamera.pressed = false;
      break;
    case SDLK_F5:
      Global::quickRepeater.pressed = false;
      break;
    case SDLK_ESCAPE:
      Global::inputEsc.pressed = false;
      break;
    case SDLK_RETURN:
      if (event.key.keysym.mod & KMOD_ALT)
        Global::inputFullscreen.pressed = false;
      break;
    }
  }
                break;

  case SDL_MOUSEBUTTONDOWN: {
    switch (event.button.button) {
    case SDL_BUTTON_LEFT:
      break;
    case SDL_BUTTON_RIGHT:
      break;
    }
  }
                          break;

  case SDL_MOUSEBUTTONUP: {
    switch (event.button.button) {
    case SDL_BUTTON_LEFT:
      break;
    case SDL_BUTTON_RIGHT:
      break;
    }
  }
                        break;

  case SDL_MOUSEMOTION: {
    Global::inputUseController = false;
    Global::inputCursorPosX = event.motion.x;
    Global::inputCursorPosY = event.motion.y;
  }
                      break;

  case SDL_CONTROLLERBUTTONDOWN:
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_A:
      break;
    case SDL_CONTROLLER_BUTTON_B:
      break;
    case SDL_CONTROLLER_BUTTON_X:
      break;
    case SDL_CONTROLLER_BUTTON_Y:
      break;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
      break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
      break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
      break;
    }
    break;
  case SDL_CONTROLLERBUTTONUP:
    switch (event.cbutton.button) {
    case SDL_CONTROLLER_BUTTON_A:
      break;
    case SDL_CONTROLLER_BUTTON_B:
      break;
    case SDL_CONTROLLER_BUTTON_X:
      break;
    case SDL_CONTROLLER_BUTTON_Y:
      break;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
      break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
      break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
      break;
    }
    break;

  case SDL_JOYAXISMOTION:
    switch (event.caxis.axis) {
    case SDL_CONTROLLER_AXIS_LEFTX:
      break;
    case SDL_CONTROLLER_AXIS_LEFTY:
      break;
    case SDL_CONTROLLER_AXIS_RIGHTX:
      break;
    case SDL_CONTROLLER_AXIS_RIGHTY:
      break;
    case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
      break;
    case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
      break;
    }
    break;

  case SDL_CONTROLLERDEVICEADDED: {
    if (Global::gameController == nullptr && SDL_IsGameController(0))
      Global::gameController = SDL_GameControllerOpen(0);
  }
                                break;

  case SDL_CONTROLLERDEVICEREMOVED: {
    SDL_GameControllerClose(Global::gameController);
    Global::gameController = nullptr;
  }
                                  break;

                                  /*case SDL_TEXTINPUT:
                                    strcat(Global::inputText, event.text.text);
                                    break;*/
  case SDL_WINDOWEVENT: {
    switch (event.window.event) {
    case SDL_WINDOWEVENT_SIZE_CHANGED: {
      Global::settings.graphicsResolutionWidth = event.window.data1;
      Global::settings.graphicsResolutionHeight = event.window.data2;
      glViewport(0, 0, Global::settings.graphicsResolutionWidth, Global::settings.graphicsResolutionHeight);
    }
                                     break;
    }
  }
                      break;
  }

}

static void postPollEventKeyInput(KeyInput& keyInput) {
  if (keyInput.pressed && !keyInput.pressedLastFrame) {
    keyInput.down = true;
    keyInput.toggle = !keyInput.toggle;
  }
  else if (!keyInput.pressed && keyInput.pressedLastFrame) {
    keyInput.up = true;
  }
}

void Input::postPollEvent() {
  postPollEventKeyInput(Global::inputA);
  postPollEventKeyInput(Global::inputD);
  postPollEventKeyInput(Global::inputW);
  postPollEventKeyInput(Global::inputS);
  postPollEventKeyInput(Global::inputE);
  postPollEventKeyInput(Global::inputC);
  postPollEventKeyInput(Global::inputKP0);
  postPollEventKeyInput(Global::inputKP1);
  postPollEventKeyInput(Global::inputKP2);
  postPollEventKeyInput(Global::inputKP3);
  postPollEventKeyInput(Global::inputKP4);
  postPollEventKeyInput(Global::inputKP5);
  postPollEventKeyInput(Global::inputKP6);
  postPollEventKeyInput(Global::inputKP7);
  postPollEventKeyInput(Global::inputKP8);
  postPollEventKeyInput(Global::inputKP9);
  postPollEventKeyInput(Global::inputFullscreen);
  postPollEventKeyInput(Global::inputPause);
  postPollEventKeyInput(Global::inputWireframe);
  postPollEventKeyInput(Global::inputDebug);
  postPollEventKeyInput(Global::inputEsc);
  postPollEventKeyInput(Global::debugCamera);
  postPollEventKeyInput(Global::quickRepeater);
}

void Input::proccessInputEvents() {
  if (Global::inputFullscreen.up) {
    Global::settings.graphicsFullscreen = Global::inputFullscreen.toggle ? FullscreenMode::windowedFullscreen : FullscreenMode::windowed;
    switch (Global::settings.graphicsFullscreen)
    {
    case FullscreenMode::windowed:
      SDL_SetWindowFullscreen(Global::window, 0);
      break;
    case FullscreenMode::fullscreen:
      SDL_SetWindowFullscreen(Global::window, SDL_WINDOW_FULLSCREEN);
      break;
    case FullscreenMode::windowedFullscreen:
      SDL_SetWindowFullscreen(Global::window, SDL_WINDOW_FULLSCREEN_DESKTOP);
      break;
    }
  }

  if (Global::inputPause.up) {
    Global::pauseAudio = Global::inputPause.toggle;
    Sound::pauseAudioDevice(Global::pauseAudio);
  }

  if (Global::inputKP0.pressed && !Global::inputKP0.pressedLastFrame)
  {
    Global::vstToneAssignment = 0;
    Global::toneAssignment = Global::time;
  }
  if (Global::inputKP1.pressed && !Global::inputKP1.pressedLastFrame)
  {
    Global::vstToneAssignment = 1;
    Global::toneAssignment = Global::time;
  }
  if (Global::inputKP2.pressed && !Global::inputKP2.pressedLastFrame)
  {
    Global::vstToneAssignment = 2;
    Global::toneAssignment = Global::time;
  }
  if (Global::inputKP3.pressed && !Global::inputKP3.pressedLastFrame)
  {
    Global::vstToneAssignment = 3;
    Global::toneAssignment = Global::time;
  }
  if (Global::inputKP4.pressed && !Global::inputKP4.pressedLastFrame)
  {
    Global::vstToneAssignment = 4;
    Global::toneAssignment = Global::time;
  }
  if (Global::inputKP5.pressed && !Global::inputKP5.pressedLastFrame)
  {
    Global::vstToneAssignment = 5;
    Global::toneAssignment = Global::time;
  }
  if (Global::inputKP6.pressed && !Global::inputKP6.pressedLastFrame)
  {
    Global::vstToneAssignment = 6;
    Global::toneAssignment = Global::time;
  }
  if (Global::inputKP7.pressed && !Global::inputKP7.pressedLastFrame)
  {
    Global::vstToneAssignment = 7;
    Global::toneAssignment = Global::time;
  }
  if (Global::inputKP8.pressed && !Global::inputKP8.pressedLastFrame)
  {
    Global::vstToneAssignment = 8;
    Global::toneAssignment = Global::time;
  }
  if (Global::inputKP9.pressed && !Global::inputKP9.pressedLastFrame)
  {
    Global::vstToneAssignment = 9;
    Global::toneAssignment = Global::time;
  }
}