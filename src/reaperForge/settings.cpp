
#include "settings.h"
#include "global.h"
#include "helper.h"

#include "SDL2/SDL_video.h"

DisplayMode Settings::displayMode;
bool Settings::pauseAudio;
u32 Settings::windowWidth;
u32 Settings::windowHeight;

std::vector<Settings::Resolution> Settings::supportedDisplayResolutions;
i32 Settings::selectedSupportedDisplayResolution = -1;

static std::vector<Settings::Resolution> getSupportedDisplayResolutions()
{
  std::vector<Settings::Resolution> supportedDisplayResolutions;

  const i32 displayCount = SDL_GetNumVideoDisplays();
  for (i32 displayIdx = 0; displayIdx <= 1; ++displayIdx)
  {
    const i32 modeCount = SDL_GetNumDisplayModes(displayIdx);
    for (i32 modeIdx = 0; modeIdx <= modeCount; ++modeIdx)
    {
      SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

      if (SDL_GetDisplayMode(displayIdx, modeIdx, &mode) == 0)
      {
        const size_t size = supportedDisplayResolutions.size();
        if (size >= 1 && (supportedDisplayResolutions.at(size - 1).w == mode.w && supportedDisplayResolutions.at(size - 1).h))
          continue;

        const Settings::Resolution resolution{
          .w = mode.w,
          .h = mode.h,
        };
        supportedDisplayResolutions.push_back(resolution);
      }
    }
  }

  return supportedDisplayResolutions;
}

void Settings::load()
{
  Settings::displayMode = Global::displayMode;
  Settings::windowWidth = Global::windowWidth;
  Settings::windowHeight = Global::windowHeight;

  Settings::supportedDisplayResolutions = getSupportedDisplayResolutions();
}

void Settings::save()
{
}
