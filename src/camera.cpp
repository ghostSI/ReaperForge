#include "camera.h"

#include "global.h"

void Camera::init() {
  Global::cameraMat.m30 = -6.3f;
  Global::cameraMat.m31 = -7.0f;
  Global::cameraMat.m32 = -8.16f;
}

void Camera::tick() {
  if (Global::inputD.pressed)
    Global::cameraMat.m30 -= 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
  else if (Global::inputA.pressed)
    Global::cameraMat.m30 += 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
  if (Global::inputW.pressed)
    Global::cameraMat.m32 += 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
  else if (Global::inputS.pressed)
    Global::cameraMat.m32 -= 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
  if (Global::inputE.pressed)
    Global::cameraMat.m31 -= 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
  else if (Global::inputC.pressed)
    Global::cameraMat.m31 += 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
}