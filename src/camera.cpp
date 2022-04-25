#include "camera.h"

#include "global.h"

void Camera::init() {
  Global::cameraMat.m30 = -6.3f;
  Global::cameraMat.m31 = -7.0f;
  Global::cameraMat.m32 = -8.16f;
}

void Camera::tick() {
  if (Global::debugCamera.toggle)
  {
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

    Global::cameraMat.m30 = cameraTarget.v0;
    Global::cameraMat.m31 = cameraTarget.v1;
    Global::cameraMat.m32 = cameraTarget.v2;
  }



}