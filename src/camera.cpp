#include "camera.h"

#include "global.h"
#include "opengl.h"
#include "shader.h"
#include "helper.h"

static vec3 cameraTarget;
static vec3 cameraPosition =
{
  .v0 = -6.3f,
  .v1 = -7.0f,
  .v2 = -9.16f
};

void Camera::init() {
}

static void tickCameraMovement()
{
  vec3 cameraDesiredDirection =
  {
    .v0 = cameraTarget.v0 - cameraPosition.v0,
    .v1 = cameraTarget.v1 - cameraPosition.v1,
    .v2 = cameraTarget.v2 - cameraPosition.v2,
  };

  { 
    const f32 cameraDesiredDirectionLength = VecMath::length(cameraDesiredDirection);
    cameraDesiredDirection = VecMath::norm(cameraDesiredDirection);
    if (cameraDesiredDirectionLength < Const::cameraBreakRadius)
    {
      cameraDesiredDirection = VecMath::multipicate(cameraDesiredDirection, Const::cameraMaximumVelocity * (cameraDesiredDirectionLength / Const::cameraBreakRadius));
    }
    else
    {
      cameraDesiredDirection = VecMath::multipicate(cameraDesiredDirection, Const::cameraMaximumVelocity);
    }
  }

  const vec3 cameraVelocity = VecMath::truncate(cameraDesiredDirection, Const::cameraMaximumVelocity);

  const vec3 framePosChange = VecMath::multipicate(cameraVelocity, Global::frameDelta / 1000.0f);
  cameraPosition.v0 += framePosChange.v0;
  cameraPosition.v1 += framePosChange.v1;
  cameraPosition.v2 += framePosChange.v2;
}

void Camera::tick() {
  { // calc the camera Target
    const f32 oggElapsed = Global::time - Global::oggStartTime;

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
      cameraTarget.v1 = -Const::cameraXFactor * f32(width) * 0.5f - Const::cameraXOffset;
      cameraTarget.v2 = -Const::cameraYFactor * f32(width) * 0.5f - Const::cameraYOffset;
    }
  }

  tickCameraMovement();

  { // set camera Matrix
    mat4 rotMatX;
    rotMatX.m11 = cos(Const::cameraXRotation);
    rotMatX.m12 = sin(Const::cameraXRotation);
    rotMatX.m21 = -sin(Const::cameraXRotation);
    rotMatX.m22 = cos(Const::cameraXRotation);

    mat4 rotMatY;
    rotMatY.m00 = cos(Const::cameraYRotation);
    rotMatY.m02 = sin(Const::cameraYRotation);
    rotMatY.m20 = -sin(Const::cameraYRotation);
    rotMatY.m22 = cos(Const::cameraYRotation);

    mat4 translation;
    translation.m30 = cameraPosition.v0;
    translation.m31 = cameraPosition.v1;
    translation.m32 = cameraPosition.v2;

    Global::cameraMat = VecMath::multipicate(VecMath::multipicate(rotMatX, rotMatY), translation);
  }


  if (Global::debugCamera.toggle)
  {
    static vec3 debugCameraPosition = cameraTarget;

    if (Global::inputD.pressed)
      debugCameraPosition.v0 -= 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
    else if (Global::inputA.pressed)
      debugCameraPosition.v0 += 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
    if (Global::inputW.pressed)
      debugCameraPosition.v2 += 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
    else if (Global::inputS.pressed)
      debugCameraPosition.v2 -= 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
    if (Global::inputE.pressed)
      debugCameraPosition.v1 -= 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;
    else if (Global::inputC.pressed)
      debugCameraPosition.v1 += 0.001f * Global::settingsHighwaySpeedMultiplier * Global::frameDelta;

    mat4 translation;
    translation.m30 = debugCameraPosition.v0;
    translation.m31 = debugCameraPosition.v1;
    translation.m32 = debugCameraPosition.v2;

    Global::cameraMat = translation;
  }
}

static void drawCircle()
{
  const i32 circleSegments = 32;
  const f32 r = 0.1f;
  GLfloat v[circleSegments * 5];
  for (i32 i = 0; i < circleSegments; ++i)
  {
    const f32 alpha = 2.0f * PI * f32(i) / f32(circleSegments);
    v[i * 5] = r * cosf(alpha); // x
    v[i * 5 + 1] = r * sinf(alpha); // y
    v[i * 5 + 2] = 0.0f; // z
    v[i * 5 + 3] = 0.0f; // u
    v[i * 5 + 4] = 0.0f; // v
  }

  glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLE_FAN, 0, circleSegments);
}

void Camera::render()
{
  if (!Global::debugCamera.toggle)
    return;

  const GLuint shader = Shader::useShader(Shader::Stem::defaultWorld);

  {
    mat4 modelMat;
    modelMat.m30 = -cameraTarget.v0;
    modelMat.m31 = -cameraTarget.v1;
    modelMat.m32 = -cameraTarget.v2;
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
    glUniform4f(glGetUniformLocation(shader, "color"), 1.0f, 0.0f, 0.0f, 1.0f);
    drawCircle();
  }

  {
    mat4 modelMat;
    modelMat.m30 = -cameraPosition.v0;
    modelMat.m31 = -cameraPosition.v1;
    modelMat.m32 = -cameraPosition.v2;
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
    glUniform4f(glGetUniformLocation(shader, "color"), 0.0f, 1.0f, 0.0f, 1.0f);
    drawCircle();
  }
}