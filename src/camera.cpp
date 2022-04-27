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

static void tickSteeringBehaviorArive()
{
  static vec3 kinematikSpeed;
  static vec3 steeringLinear;
  f32 targetVelocity = 0.0f;

  const f32 steeringLinearMax = 2.0f;
  const f32 kinematikMaxSpeed = 2.0f;
  const f32 breakRadius = 2.0f;
  const f32 timeToTarget = 1.0f;

  const vec3 direction =
  {
    .v0 = cameraTarget.v0 - cameraPosition.v0,
    .v1 = cameraTarget.v1 - cameraPosition.v1,
    .v2 = cameraTarget.v2 - cameraPosition.v2,
  };

  const f32 directionLength = VecMath::length(direction);
  if (directionLength > breakRadius)
  {
    targetVelocity = kinematikMaxSpeed;
  }
  else
  {
    targetVelocity = kinematikMaxSpeed * directionLength / breakRadius;
  }

  const vec3 normDirection = VecMath::norm(direction);
  const vec3 velocity = {
    .v0 = normDirection.v0 * targetVelocity,
    .v1 = normDirection.v1 * targetVelocity,
    .v2 = normDirection.v2 * targetVelocity
  };

  steeringLinear = vec3{
      .v0 = (velocity.v0 - kinematikSpeed.v0) / timeToTarget,
      .v1 = (velocity.v1 - kinematikSpeed.v1) / timeToTarget,
      .v2 = (velocity.v2 - kinematikSpeed.v2) / timeToTarget
  };

  //const f32 velocityLength = VecMath::length(velocity);

  kinematikSpeed = VecMath::truncate(kinematikSpeed, kinematikMaxSpeed);
  steeringLinear = VecMath::truncate(steeringLinear, steeringLinearMax);
  //update position und orientierung
  const vec3 frameSpeedChange = VecMath::multipicate(steeringLinear, Global::frameDelta / 1000.0f);
  kinematikSpeed.v0 += frameSpeedChange.v0;
  kinematikSpeed.v1 += frameSpeedChange.v1;
  kinematikSpeed.v2 += frameSpeedChange.v2;
  const vec3 framePosChange = VecMath::multipicate(kinematikSpeed, Global::frameDelta / 1000.0f);
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
      cameraTarget.v1 = -1.3f * f32(width) * 0.5f - 1.5f;
      cameraTarget.v2 = -1.3f * f32(width) * 0.5f - 3.0f;
    }
  }

  tickSteeringBehaviorArive();

  { // set camera Matrix
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

void Camera::render()
{
  if (!Global::debugCamera.toggle)
    return;

  const GLuint shader = Shader::useShader(Shader::Stem::defaultWorld);

  { // cameraTarget
    mat4 modelMat;
    modelMat.m30 = -cameraTarget.v0;
    modelMat.m31 = -cameraTarget.v1;
    modelMat.m32 = -cameraTarget.v2;

    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
    glUniform4f(glGetUniformLocation(shader, "color"), 1.0f, 0.0f, 0.0f, 1.0f);

    const constexpr i32 circleSegments = 32;
    const f32 r = 0.1f;
    GLfloat v[circleSegments * 5]{};

    for (i32 i = 0; i < circleSegments; ++i)
    {
      const f32 alpha = 2.0f * 3.1415926f * f32(i) / f32(circleSegments);
      v[i * 5] = r * cosf(alpha);
      v[i * 5 + 1] = r * sinf(alpha);
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, circleSegments);
  }

  { // cameraPosition
    mat4 modelMat;
    modelMat.m30 = -cameraPosition.v0;
    modelMat.m31 = -cameraPosition.v1;
    modelMat.m32 = -cameraPosition.v2;

    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
    glUniform4f(glGetUniformLocation(shader, "color"), 0.0f, 1.0f, 0.0f, 1.0f);

    const constexpr i32 circleSegments = 32;
    const f32 r = 0.1f;
    GLfloat v[circleSegments * 5]{};

    for (i32 i = 0; i < circleSegments; ++i)
    {
      const f32 alpha = 2.0f * 3.1415926f * f32(i) / f32(circleSegments);
      v[i * 5] = r * cosf(alpha);
      v[i * 5 + 1] = r * sinf(alpha);
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, circleSegments);
  }
}