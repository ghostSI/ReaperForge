#include "camera.h"

#include "global.h"

void Camera::tick() {
  if (Global::inputD.pressed)
    Global::cameraMat.m03 -= 0.001f * Global::frameDelta;
  else if (Global::inputA.pressed)
    Global::cameraMat.m03 += 0.001f * Global::frameDelta;
  if (Global::inputW.pressed)
    Global::cameraMat.m23 += 0.001f * Global::frameDelta;
  else if (Global::inputS.pressed)
    Global::cameraMat.m23 -= 0.001f * Global::frameDelta;
  if (Global::inputE.pressed)
    Global::cameraMat.m13 -= 0.001f * Global::frameDelta;
  else if (Global::inputC.pressed)
    Global::cameraMat.m13 += 0.001f * Global::frameDelta;
}