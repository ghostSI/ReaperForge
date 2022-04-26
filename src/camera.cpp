#include "camera.h"

#include "global.h"


void Camera::init() {
}

void Camera::tick() {
  static vec3 cameraPosition = {
  .v0 = -6.3f,
  .v1 = -7.0f,
  .v2 = -9.16f
  };

  if (Global::debugCamera.toggle)
  {
    if (Global::inputD.pressed)
      cameraPosition.v0 -= 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
    else if (Global::inputA.pressed)
      cameraPosition.v0 += 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
    if (Global::inputW.pressed)
      cameraPosition.v2 += 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
    else if (Global::inputS.pressed)
      cameraPosition.v2 -= 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
    if (Global::inputE.pressed)
      cameraPosition.v1 -= 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
    else if (Global::inputC.pressed)
      cameraPosition.v1 += 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;

    const f32 rotX = 0.05f;
    mat4 rotMatX;
    rotMatX.m11 = cos(rotX);
    rotMatX.m12 = sin(rotX);
    rotMatX.m21 = -sin(rotX);
    rotMatX.m22 = cos(rotX);

    const f32 rotY = 0.19f;
    mat4 rotMatY;
    rotMatY.m00 = cos(rotY);
    rotMatY.m02 = sin(rotY);
    rotMatY.m20 = -sin(rotY);
    rotMatY.m22 = cos(rotY);

    mat4 translation;
    translation.m30 = cameraPosition.v0;
    translation.m31 = cameraPosition.v1;
    translation.m32 = cameraPosition.v2;

    Global::cameraMat = VecMath::multipicate(VecMath::multipicate(rotMatX, rotMatY), translation);
  }
  else
  {
    const f32 oggElapsed = Global::time - Global::oggStartTime;

    vec3 cameraTarget;

    for (i32 i = 0; i < i32(Global::songTrack.transcriptionTrack.anchors.size()) - 3; ++i)
    {
      const Song::TranscriptionTrack::Anchor& anchor0 = Global::songTrack.transcriptionTrack.anchors[i];
      const Song::TranscriptionTrack::Anchor& anchor1 = Global::songTrack.transcriptionTrack.anchors[i + 1];

      const f32 noteTime0 = -anchor0.time + oggElapsed;
      const f32 noteTime1 = -anchor1.time + oggElapsed;

      if (noteTime0 < 0.0f)
        continue;
      if (noteTime1 > 0.0f)
        continue;

      const Song::TranscriptionTrack::Anchor& anchor2 = Global::songTrack.transcriptionTrack.anchors[i + 2];

      i32 left = min_(min_(anchor0.fret, anchor1.fret), anchor2.fret);
      i32 right = max_(max_(anchor0.fret + anchor0.width, anchor1.fret + anchor1.width), anchor2.fret + anchor2.width);
      i32 width = right - left;
      cameraTarget.v0 = -f32(left - 1 + right - 1) * 0.5f;
      cameraTarget.v1 = -1.3f * f32(width) * 0.5f - 1.5f;
      cameraTarget.v2 = -1.3f * f32(width) * 0.5f - 3.0f;

    }

    const f32 rotX = 0.05f;
    mat4 rotMatX;
    rotMatX.m11 = cos(rotX);
    rotMatX.m12 = sin(rotX);
    rotMatX.m21 = -sin(rotX);
    rotMatX.m22 = cos(rotX);

    const f32 rotY = 0.19f;
    mat4 rotMatY;
    rotMatY.m00 = cos(rotY);
    rotMatY.m02 = sin(rotY);
    rotMatY.m20 = -sin(rotY);
    rotMatY.m22 = cos(rotY);

    mat4 translation;
    translation.m30 = cameraTarget.v0;
    translation.m31 = cameraTarget.v1;
    translation.m32 = cameraTarget.v2;

    Global::cameraMat = VecMath::multipicate(VecMath::multipicate(rotMatX, rotMatY), translation);
  }
}