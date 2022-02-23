#include "input.h"

#include "type.h"
#include "global.h"
#include "helper.h"
#include "sound.h"

#include "SDL2/SDL_events.h"

static void proccessInputEvents()
{
  const DisplayMode displayMode = Global::inputFullscreen.toggle ? DisplayMode::fullscreen : DisplayMode::windowed;
  if (Global::inputFullscreen.up)
  {
    Global::displayMode = displayMode;
    SDL_SetWindowFullscreen(Global::window, displayMode == DisplayMode::fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_FULLSCREEN_DESKTOP);
  }

  if (Global::inputPause.up)
  {
    Global::pauseAudio = Global::inputPause.toggle;
    Sound::setPauseAudio(Global::pauseAudio);
  }
}

static void prePollEventKeyInput(KeyInput& keyInput)
{
  keyInput.pressedLastFrame = keyInput.pressed;
  keyInput.down = false;
  keyInput.up = false;
}

void Input::prePollEvent()
{
  prePollEventKeyInput(Global::inputFullscreen);
  prePollEventKeyInput(Global::inputPause);
}

void Input::pollEvent(SDL_Event& event)
{
  switch (event.type)
  {
  case SDL_QUIT:
    Global::appQuit = true;
    break;

  case SDL_KEYDOWN:
  {
    switch (event.key.keysym.sym)
    {
    case SDLK_a:
      break;
    case SDLK_d:
      break;
    case SDLK_w:
      break;
    case SDLK_s:
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
    case SDLK_RETURN:
      if (event.key.keysym.mod & KMOD_ALT)
        Global::inputFullscreen.pressed = true;
      break;
    }
  }
  break;

  case SDL_KEYUP:
  {
    switch (event.key.keysym.sym)
    {
    case SDLK_a:
      break;
    case SDLK_d:
      break;
    case SDLK_w:
      break;
    case SDLK_s:
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
    case SDLK_RETURN:
      if (event.key.keysym.mod & KMOD_ALT)
        Global::inputFullscreen.pressed = false;
      break;
    }
  }
  break;

  case SDL_MOUSEBUTTONDOWN:
  {
    switch (event.button.button)
    {
    case SDL_BUTTON_LEFT:
      break;
    case SDL_BUTTON_RIGHT:
      break;
    }
  }
  break;

  case SDL_MOUSEBUTTONUP:
  {
    switch (event.button.button)
    {
    case SDL_BUTTON_LEFT:
      break;
    case SDL_BUTTON_RIGHT:
      break;
    }
  }
  break;

  case SDL_MOUSEMOTION:
  {
    Global::inputUseController = false;
    Global::inputCursorPosX = event.motion.x;
    Global::inputCursorPosY = event.motion.y;
  }
  break;

  case SDL_CONTROLLERBUTTONDOWN:
    switch (event.cbutton.button)
    {
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
    switch (event.cbutton.button)
    {
    case  SDL_CONTROLLER_BUTTON_A:
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
    switch (event.caxis.axis)
    {
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

  case SDL_CONTROLLERDEVICEADDED:
  {
    if (Global::gameController == nullptr && SDL_IsGameController(0))
      Global::gameController = SDL_GameControllerOpen(0);
  }
  break;

  case SDL_CONTROLLERDEVICEREMOVED:
  {
    SDL_GameControllerClose(Global::gameController);
    Global::gameController = nullptr;
  }
  break;

  /*case SDL_TEXTINPUT:
    strcat(Global::inputText, event.text.text);
    break;*/
  case SDL_WINDOWEVENT:
  {
    switch (event.window.event)
    {
    case SDL_WINDOWEVENT_RESIZED:
    {
      Global::windowWidth = event.window.data1;
      Global::windowHeight = event.window.data2;
      glViewport(0, 0, Global::windowWidth, Global::windowHeight);
    }
    break;
    }
  }
  break;
  }

  proccessInputEvents();
}

static void postPollEventKeyInput(KeyInput& keyInput)
{
  if (keyInput.pressed && !keyInput.pressedLastFrame)
  {
    keyInput.down = true;
    keyInput.toggle = !keyInput.toggle;
  }
  else if (!keyInput.pressed && keyInput.pressedLastFrame)
  {
    keyInput.up = true;
  }
}

void Input::postPollEvent()
{
  postPollEventKeyInput(Global::inputFullscreen);
}
