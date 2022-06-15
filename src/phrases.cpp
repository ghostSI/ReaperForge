
#include "phrases.h"

#include "global.h"
#include "opengl.h"
#include "shader.h"
#include "song.h"

#include "SDL2/SDL.h"


void Phrases::tick()
{
  if (Global::songTrack.phraseIterations.size() >= 1)
  {
    const i32 left = i32(0.1f * Global::settings.graphicsResolutionWidth);
    const i32 right = i32(0.9f * Global::settings.graphicsResolutionWidth);
    const i32 top = i32(0.04f * Global::settings.graphicsResolutionHeight);
    const i32 bottom = i32(0.135f * Global::settings.graphicsResolutionHeight);

    if (Global::inputCursorPosX >= left && Global::inputCursorPosX <= right && Global::inputCursorPosY >= top && Global::inputCursorPosY <= bottom)
    {
      static SDL_Cursor* cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
      SDL_SetCursor(cursor);

      if (Global::inputLmb.pressed && !Global::inputLmb.pressedLastFrame)
      {
        const f32 progress = f32(Global::inputCursorPosX - left) / f32(right - left);

        const i64 prog = (u64(progress * Global::musicBufferLength) / sizeof(f32)) * sizeof(f32); // make sure we don't end somewhere between two f32.

        Global::musicBufferPosition = &Global::musicBuffer[prog];

        Global::musicTimeElapsed = (progress * f32(Global::musicBufferLength)) / f32(2 * Global::settings.audioSampleRate * sizeof(f32));
      }

      return;
    }
  }

  static SDL_Cursor* cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
  SDL_SetCursor(cursor);
}

void Phrases::render()
{
  GLuint shader = Shader::useShader(Shader::Stem::phrasesScreen);

  i32 maxDifficulty = 0;
  for (const Song::Phrase& phase : Global::songTrack.phrases)
    if (phase.maxDifficulty > maxDifficulty)
      maxDifficulty = phase.maxDifficulty;

  for (i32 i = 0; i < i32(Global::songTrack.phraseIterations.size()) - 1; ++i)
  {
    const Song::PhraseIteration& phraseIteration0 = Global::songTrack.phraseIterations[i];
    const Song::PhraseIteration& phraseIteration1 = Global::songTrack.phraseIterations[i + 1];

    const Song::Phrase& phase = Global::songTrack.phrases[phraseIteration0.phraseId];

    f32 begin = phraseIteration0.time / Global::songInfos[Global::songSelected].manifestInfos[Global::manifestSelected].songLength;
    f32 end = phraseIteration1.time / Global::songInfos[Global::songSelected].manifestInfos[Global::manifestSelected].songLength;
    f32 difficulty = f32(phase.maxDifficulty) / f32(maxDifficulty);

    const f32 left = -0.7985f + begin * 1.6f;
    const f32 right = -0.8015f + end * 1.6f;
    const f32 posZ = 0.3f;

    const f32 progress = map(Global::musicTimeElapsed, phraseIteration0.time, phraseIteration1.time, 0.0f, 1.0f);
    glUniform1f(glGetUniformLocation(shader, "progress"), progress);

    {
      const f32 top = 0.75f + difficulty * 0.17f;
      const f32 bottom = 0.75f;

      // for sprites triangleStrip: 4 Verts + UV. Format: x,y,z,u,v
      const GLfloat v[] = {
        left , top, posZ, 0.0f, 1.0f,
        right, top, posZ, 1.0f, 1.0f,
        left, bottom, posZ, 0.0f, 0.0f,
        right, bottom, posZ, 1.0f, 0.0f,
      };

      glUniform4f(glGetUniformLocation(shader, "color"), Global::settings.highwayPhraseColor[0].v0, Global::settings.highwayPhraseColor[0].v1, Global::settings.highwayPhraseColor[0].v2, Global::settings.highwayPhraseColor[0].v3);
      glUniform4f(glGetUniformLocation(shader, "color2"), Global::settings.highwayPhraseColor[1].v0, Global::settings.highwayPhraseColor[1].v1, Global::settings.highwayPhraseColor[1].v2, Global::settings.highwayPhraseColor[1].v3);

      glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    {
      const f32 top = 0.745f;
      const f32 bottom = 0.73f;

      // for sprites triangleStrip: 4 Verts + UV. Format: x,y,z,u,v
      const GLfloat v[] = {
        left , top, posZ, 0.0f, 1.0f,
        right, top, posZ, 1.0f, 1.0f,
        left, bottom, posZ, 0.0f, 0.0f,
        right, bottom, posZ, 1.0f, 0.0f,
      };

      glUniform4f(glGetUniformLocation(shader, "color"), Global::settings.highwayPhraseColor[2].v0, Global::settings.highwayPhraseColor[2].v1, Global::settings.highwayPhraseColor[2].v2, Global::settings.highwayPhraseColor[2].v3);
      glUniform4f(glGetUniformLocation(shader, "color2"), Global::settings.highwayPhraseColor[3].v0, Global::settings.highwayPhraseColor[3].v1, Global::settings.highwayPhraseColor[3].v2, Global::settings.highwayPhraseColor[3].v3);

      glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
  }
}
