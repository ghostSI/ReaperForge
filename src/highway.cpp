#include "highway.h"

#include "data.h"
#include "type.h"
#include "chords.h"
#include "opengl.h"
#include "shader.h"
#include "settings.h"
#include "helper.h"
#include "global.h"
#include "psarc.h"
#include "song.h"
#include "font.h"
#include "sound.h"

#include <string.h>

static i32 instrumentStringCount = 6;
static i32 instrumentStringOffset = 0;
static i32 instrumentFirstWoundString = 3;
static vec4* instrumentStringColors = Global::settings.instrumentGuitarStringColor;

static void drawGround()
{
  GLuint shader = Shader::useShader(Shader::Stem::ground);

  mat4 modelMat;
  glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
  glUniform4f(glGetUniformLocation(shader, "color"), 0.2f, 0.2f, 0.6f, 1.0f);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::ground), Data::Geometry::ground, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::ground) / (sizeof(float) * 5));
}

static void drawFrets()
{
  GLuint shader = Shader::useShader(Shader::Stem::defaultWorld);

  const f32 oggElapsed = Global::time - Global::oggStartTime;

  i32 chordBoxLeft = 24;
  i32 chordBoxRight = 0;
  i32 chordBoxLeftNext = 24;
  i32 chordBoxRightNext = 0;

  for (i32 i = 0; i < i32(Global::songTrack.transcriptionTrack.anchors.size()) - 2; ++i)
  {
    const Song::TranscriptionTrack::Anchor& anchor0 = Global::songTrack.transcriptionTrack.anchors[i];
    const Song::TranscriptionTrack::Anchor& anchor1 = Global::songTrack.transcriptionTrack.anchors[i + 1];

    if (oggElapsed >= anchor0.time && oggElapsed < anchor1.time)
    {
      chordBoxLeft = anchor0.fret;
      chordBoxRight = chordBoxLeft + anchor0.width;
      chordBoxLeftNext = anchor1.fret;
      chordBoxRightNext = chordBoxLeftNext + anchor1.width;
      break;
    }
  }

  mat4 modelMat;
  switch (instrumentStringCount)
  {
  case 4:
    modelMat.m11 = 0.65f;
    break;
  case 5:
    modelMat.m11 = 0.80f;
    break;
  case 6:
    modelMat.m11 = 0.92f;
    break;
  case 7:
    modelMat.m11 = 1.07f;
    break;
  default:
    assert(false);
    break;
  }

  modelMat.m31 = -0.3f;

  for (i32 i = 0; i < sizeof(Const::highwayRenderFretPosition) / sizeof(*Const::highwayRenderFretPosition); ++i)
  {
    GLuint shader;
    if (chordBoxLeft - 1 <= i && chordBoxRight - 1 >= i)
      shader = Shader::useShader(Shader::Stem::fretSilver);
    else if (chordBoxLeftNext - 1 <= i && chordBoxRightNext - 1 >= i)
      shader = Shader::useShader(Shader::Stem::fretGold);
    else
      shader = Shader::useShader(Shader::Stem::fretBronze);

    modelMat.m30 = Const::highwayRenderFretPosition[i];
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

    glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::fret), Data::Geometry::fret, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::fret) / (sizeof(float) * 5));
  }
}

static void setStringColor(GLuint shader, i32 string, f32 alpha = 1.0f)
{
  assert(string >= 0);
  assert(string <= 6);
  const vec4& colorVec = instrumentStringColors[string];
  glUniform4f(glGetUniformLocation(shader, "color"), colorVec.v0, colorVec.v1, colorVec.v2, alpha);
}

static void drawStrings()
{
  const GLuint shader = Shader::useShader(Shader::Stem::string);

  mat4 modelMat;
  modelMat.m00 = 600.0f;

  for (i32 i = 0; i < instrumentStringCount; ++i)
  {
    const f32 alpha = i >= instrumentFirstWoundString ? 1.0f : 0.0f;
    setStringColor(shader, i, alpha);
    modelMat.m31 = f32(i) * Const::highwayRenderStringSpacing;
    modelMat.m11 = 1.0f + i * Const::highwayRenderStringGaugeMultiplier;
    modelMat.m22 = 1.0f + i * Const::highwayRenderStringGaugeMultiplier;
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::string), Data::Geometry::string, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::string) / (sizeof(float) * 5));
  }
}

static void drawNote(const Song::TranscriptionTrack::Note& note, f32 noteTime, f32 fretboardNoteDistance[7][24], i32 chordBoxLeft, i32 chordBoxWidth)
{
  const GLuint shader = Shader::useShader(Shader::Stem::defaultWorld);

  if (noteTime > -1.0f)
  {
    if (fretboardNoteDistance[note.string][note.fret] == 0.0f || fretboardNoteDistance[note.string][note.fret] < noteTime)
      fretboardNoteDistance[note.string][note.fret] = noteTime;
  }

  if (noteTime < 0.0f)
  {
    if (note.fret == 0)
    {
      setStringColor(shader, 5 - note.string + instrumentStringOffset);

      {
        mat4 modelMat;
        modelMat.m30 = Const::highwayRenderFretPosition[chordBoxLeft - 1] + 0.05f;
        modelMat.m31 = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
        modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

        glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::zeroLeft), Data::Geometry::zeroLeft, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::zeroLeft) / (sizeof(float) * 5));
      }

      {
        mat4 modelMat;
        modelMat.m00 = 16.0f;
        modelMat.m30 = Const::highwayRenderFretPosition[chordBoxLeft - 1] + 0.266f;
        modelMat.m31 = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
        modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

        glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::zeroMiddle), Data::Geometry::zeroMiddle, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::zeroMiddle) / (sizeof(float) * 5));
      }

      {
        mat4 modelMat;
        modelMat.m30 = Const::highwayRenderFretPosition[chordBoxLeft - 1] + 3.73f;
        modelMat.m31 = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
        modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

        glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::zeroRight), Data::Geometry::zeroRight, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::zeroRight) / (sizeof(float) * 5));
      }

      {
        mat4 modelMat;
        modelMat.m30 = Const::highwayRenderFretPosition[chordBoxLeft - 1] + 0.5f * Const::highwayRenderFretPosition[chordBoxWidth];
        modelMat.m31 = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
        modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
      }
    }
    else
    {
      const f32 x = Const::highwayRenderFretPosition[note.fret - 1] + 0.5f * (Const::highwayRenderFretPosition[note.fret] - Const::highwayRenderFretPosition[note.fret - 1]);

      mat4 modelMat;
      modelMat.m30 = x;
      modelMat.m31 = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
      modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
      glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

      setStringColor(shader, 5 - note.string + instrumentStringOffset);

      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::note), Data::Geometry::note, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::note) / (sizeof(float) * 5));
    }

    if (note.hammerOn)
    {
      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::hammerOn), Data::Geometry::hammerOn, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::hammerOn) / (sizeof(float) * 5));
    }
    if (note.harmonic)
    {
      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::harmonic), Data::Geometry::harmonic, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::harmonic) / (sizeof(float) * 5));
    }
    if (note.mute)
    {
      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::fretMute), Data::Geometry::fretMute, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::fretMute) / (sizeof(float) * 5));
    }
    if (note.palmMute)
    {
      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::palmMute), Data::Geometry::palmMute, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::palmMute) / (sizeof(float) * 5));
    }
    if (note.pullOff)
    {
      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::pullOff), Data::Geometry::pullOff, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::pullOff) / (sizeof(float) * 5));
    }
    if (note.slap)
    {
      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::slap), Data::Geometry::slap, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::slap) / (sizeof(float) * 5));
    }
    if (note.harmonicPinch)
    {
      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::harmonicPinch), Data::Geometry::harmonicPinch, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::harmonicPinch) / (sizeof(float) * 5));
    }
    if (note.tap)
    {
      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::tap), Data::Geometry::tap, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::tap) / (sizeof(float) * 5));
    }
  }
  if (note.sustain != 0.0f)
  {
    setStringColor(shader, 5 - note.string + instrumentStringOffset, 0.85f);

    if (note.tremolo)
    {
      const f32 x = Const::highwayRenderFretPosition[note.fret - 1] + 0.5f * (Const::highwayRenderFretPosition[note.fret] - Const::highwayRenderFretPosition[note.fret - 1]);

      mat4 modelMat;
      modelMat.m30 = x;
      modelMat.m31 = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
      modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
      glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);


      std::vector<GLfloat> vv;
      vv.insert(vv.end(), { -0.2_f32, 0.0f, 0.0f, 0.8418f, 1.0f });
      vv.insert(vv.end(), { 0.2_f32, 0.0f, 0.0f, 0.9922f, 1.0f, });
      i32 j = 0;
      for (f32 f = 0.5f * Const::highwayRenderTremoloFrequency; f < note.sustain - (0.5f * Const::highwayRenderTremoloFrequency); f += Const::highwayRenderTremoloFrequency)
      {
        const f32 sustainTime = -f * Global::settings.highwaySpeedMultiplier;

        if (j % 2 == 0)
        {
          vv.insert(vv.end(), { -0.15_f32 , 0.0f, sustainTime, 0.8418f, 0.5f });
          vv.insert(vv.end(), { 0.25_f32, 0.0f, sustainTime, 0.9922f, 0.5f, });
        }
        else
        {
          vv.insert(vv.end(), { -0.25_f32 , 0.0f, sustainTime, 0.8418f, 0.5f });
          vv.insert(vv.end(), { 0.15_f32, 0.0f, sustainTime, 0.9922f, 0.5f, });
        }

        ++j;
      }

      const f32 sustainTime = -note.sustain * Global::settings.highwaySpeedMultiplier;
      vv.insert(vv.end(), { -0.2_f32, 0.0f, sustainTime, 0.8418f, 0.0f });
      vv.insert(vv.end(), { 0.2_f32, 0.0f, sustainTime, 0.9922f, 0.0f });

      glBufferData(GL_ARRAY_BUFFER, vv.size() * sizeof(GLfloat), vv.data(), GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, vv.size() / 5);
    }
    else
    {
      const f32 x = Const::highwayRenderFretPosition[note.fret - 1] + 0.5f * (Const::highwayRenderFretPosition[note.fret] - Const::highwayRenderFretPosition[note.fret - 1]);

      mat4 modelMat;
      modelMat.m30 = x;
      modelMat.m31 = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
      //modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
      glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

      const f32 front = min_(noteTime * Global::settings.highwaySpeedMultiplier, 0.0f);
      const f32 back = (noteTime - note.sustain) * Global::settings.highwaySpeedMultiplier;

      const GLfloat v[] = {
        -0.2_f32, 0.0f, front, 0.8418f, 1.0f,
        0.2_f32, 0.0f, front, 0.9922f, 1.0f,
        -0.2_f32, 0.0f, back, 0.8418f, 0.0f,
        0.2_f32, 0.0f, back, 0.9922f, 0.0f,
      };

      glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
  }
}

static void drawNoteStand(const Song::TranscriptionTrack::Note& note, f32 noteTime)
{
  const GLuint shader = Shader::useShader(Shader::Stem::noteStand);

  mat4 modelMat;
  modelMat.m30 = Const::highwayRenderFretPosition[note.fret - 1] + 0.5f * (Const::highwayRenderFretPosition[note.fret] - Const::highwayRenderFretPosition[note.fret - 1]);
  modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
  modelMat.m31 = -0.735f * Const::highwayRenderStringSpacing;
  modelMat.m11 = 2.0f * f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing - 2.0f * modelMat.m31;
  glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

  setStringColor(shader, 5 - note.string + instrumentStringOffset);

  glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::noteStand), Data::Geometry::noteStand, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::noteStand) / (sizeof(float) * 5));
}

static void drawNoteStandZero(const Song::TranscriptionTrack::Note& note, f32 noteTime, i32 chordBoxLeft, i32 chordBoxWidth)
{
  const GLuint shader = Shader::useShader(Shader::Stem::noteStandZero);

  const f32 left = Const::highwayRenderFretPosition[chordBoxLeft - 1];
  const f32 top = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
  const f32 right = Const::highwayRenderFretPosition[chordBoxLeft + chordBoxWidth - 1];
  const f32 bottom = -0.60f * Const::highwayRenderStringSpacing;
  const f32 posZ = noteTime * Global::settings.highwaySpeedMultiplier;

  // for sprites triangleStrip: 4 Verts + UV. Format: x,y,z,u,v
  const GLfloat v[] = {
    left , top, posZ, 0.0f, 1.0f,
    right, top, posZ, 1.0f, 1.0f,
    left, bottom, posZ, 0.0f, 0.0f,
    right, bottom, posZ, 1.0f, 0.0f,
  };

  setStringColor(shader, 5 - note.string + instrumentStringOffset);

  glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void drawNotes(f32 fretboardNoteDistance[7][24])
{
  const GLuint shader = Shader::useShader(Shader::Stem::defaultWorld);

  const f32 oggElapsed = Global::time - Global::oggStartTime;

  for (i32 i = Global::songTrack.transcriptionTrack.notes.size() - 1; i >= 0; --i)
  {
    const Song::TranscriptionTrack::Note& note = Global::songTrack.transcriptionTrack.notes[i];

    const f32 noteTime = -note.time + oggElapsed;

    if (noteTime - note.sustain > 0.0f)
      continue;
    if (noteTime < Const::highwayRenderMaxFutureTime)
      continue;

    i32 currentAnchor = -1;
    for (i32 i = 0; i < Global::songTrack.transcriptionTrack.anchors.size() - 2; ++i)
    {
      const Song::TranscriptionTrack::Anchor& anchor0 = Global::songTrack.transcriptionTrack.anchors[i];
      const Song::TranscriptionTrack::Anchor& anchor1 = Global::songTrack.transcriptionTrack.anchors[i + 1];

      if (note.time >= anchor0.time && note.time < anchor1.time)
      {
        currentAnchor = i;
        break;
      }
    }
    //assert(currentAnchor >= 0);
    if (currentAnchor < 0)
      currentAnchor = 0;

    drawNote(note, noteTime, fretboardNoteDistance, Global::songTrack.transcriptionTrack.anchors[currentAnchor].fret, Global::songTrack.transcriptionTrack.anchors[currentAnchor].width);

    if (noteTime < 0.0f)
    {
      if (note.fret >= 1)  // Draw Fret Numbers for Chord
      {
        drawNoteStand(note, noteTime);

        const GLuint shader = Shader::useShader(Shader::Stem::fontWorld);
        glUniform4f(glGetUniformLocation(shader, "color"), 0.831f, 0.686f, 0.216f, 1.0f);

        const f32 x = Const::highwayRenderFretPosition[note.fret - 1] + 0.5f * (Const::highwayRenderFretPosition[note.fret] - Const::highwayRenderFretPosition[note.fret - 1]);

        Font::drawFretNumber(note.fret, x, -0.2f, noteTime * Global::settings.highwaySpeedMultiplier, 0.25f, 0.25f);
      }
      else
      {
        drawNoteStandZero(note, noteTime, Global::songTrack.transcriptionTrack.anchors[currentAnchor].fret, Global::songTrack.transcriptionTrack.anchors[currentAnchor].width);
      }
    }
  }
}

static void drawAnchor(const Song::TranscriptionTrack::Anchor& anchor, f32 noteTimeBegin, f32 noteTimeEnd)
{
  const GLuint shader = Shader::useShader(Shader::Stem::anchor);

  const f32 front = min_(noteTimeBegin * Global::settings.highwaySpeedMultiplier, 0.0f);
  const f32 back = noteTimeEnd * Global::settings.highwaySpeedMultiplier;

  for (i32 i = 0; i < anchor.width; ++i)
  {
    const f32 left = anchor.fret - 1 + i;
    const f32 right = anchor.fret + i;

    const GLfloat v[] = {
      left , -0.36f, front, 0.0f, 1.0f,
      right, -0.36f, front, 1.0f, 1.0f,
      left, -0.36f, back, 0.0f, 0.0f,
      right, -0.36f, back, 1.0f, 0.0f,
    };

    if (Const::isMarkedFret[anchor.fret + i])
      glUniform4f(glGetUniformLocation(shader, "color"), 0.10f, 0.10f, 0.40f, 0.70f);
    else
      glUniform4f(glGetUniformLocation(shader, "color"), 0.06f, 0.06f, 0.28f, 0.70f);

    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
}

static void drawAnchors()
{
  const f32 oggElapsed = Global::time - Global::oggStartTime;

  for (i32 i = 0; i < i32(Global::songTrack.transcriptionTrack.anchors.size()) - 2; ++i)
  {
    const Song::TranscriptionTrack::Anchor& anchor0 = Global::songTrack.transcriptionTrack.anchors[i];
    const Song::TranscriptionTrack::Anchor& anchor1 = Global::songTrack.transcriptionTrack.anchors[i + 1];

    const f32 noteTimeBegin = -anchor0.time + oggElapsed;
    const f32 noteTimeEnd = -anchor1.time + oggElapsed;

    if (noteTimeEnd > 0.0f)
      continue;
    if (noteTimeBegin < Const::highwayRenderMaxFutureTime)
      continue;

    drawAnchor(anchor0, noteTimeBegin, noteTimeEnd);
  }
}

static void drawChordName(i32 chordId, f32 noteTime, i32 chordBoxLeft, bool consecutiveChord)
{
  if (chordId < 0)
    return;

  f32 alpha = consecutiveChord ? 0.0f : 1.0f;

  if (noteTime > 0.0f)
  {
    if (noteTime < Const::highwayRenderDrawChordNameWaitTime)
    {
      alpha = 1.0f;
      noteTime = 0.0f;
    }
    else
    {
      alpha = (Const::highwayRenderDrawChordNameEndTime - noteTime) / (Const::highwayRenderDrawChordNameEndTime - Const::highwayRenderDrawChordNameFadeOutTime);
      noteTime -= Const::highwayRenderDrawChordNameWaitTime;
    }
  }

  const Song::ChordTemplate& chordTemplate = Global::songTrack.chordTemplates[chordId];

  const GLuint shader = Shader::useShader(Shader::Stem::fontWorld);
  glUniform4f(glGetUniformLocation(shader, "color"), 1.0, 1.0f, 1.0f, alpha);
  Font::draw(Chords::translatedName(chordTemplate.chordName).c_str(), Const::highwayRenderFretPosition[chordBoxLeft] - 1.5f, f32(instrumentStringCount + instrumentStringOffset) * Const::highwayRenderStringSpacing - 0.30f * Const::highwayRenderStringSpacing, noteTime * Global::settings.highwaySpeedMultiplier, 0.5f, 0.5f);
}

static void drawChord(const Song::TranscriptionTrack::Chord& chord, f32 noteTime, bool consecutiveChord, f32 fretboardNoteDistance[7][24])
{
  u32 fretsInChord = 0;

  i32 chordBoxLeft = 24;
  i32 chordBoxRight = 0;

  for (const Song::TranscriptionTrack::Note& note : chord.chordNotes)
  {
    if (note.fret > 0 && note.fret < chordBoxLeft)
      chordBoxLeft = note.fret;

    if (note.fret > chordBoxRight)
      chordBoxRight = note.fret;

    fretsInChord |= 1 << note.fret;
  }

  i32 currentAnchor = -1;
  for (i32 i = 0; i < Global::songTrack.transcriptionTrack.anchors.size() - 2; ++i)
  {
    const Song::TranscriptionTrack::Anchor& anchor0 = Global::songTrack.transcriptionTrack.anchors[i];
    const Song::TranscriptionTrack::Anchor& anchor1 = Global::songTrack.transcriptionTrack.anchors[i + 1];

    if (chord.time >= anchor0.time && chord.time < anchor1.time)
    {
      currentAnchor = i;
      break;
    }
  }
  //assert(currentAnchor >= 0);
  if (currentAnchor < 0)
    currentAnchor = 0;

  if (currentAnchor >= 0)
  {
    chordBoxLeft = Global::songTrack.transcriptionTrack.anchors[currentAnchor].fret;
    chordBoxRight = Global::songTrack.transcriptionTrack.anchors[currentAnchor].fret + Global::songTrack.transcriptionTrack.anchors[currentAnchor].width;
  }
  if (chordBoxRight - chordBoxLeft < 4)
    chordBoxRight = chordBoxLeft + 4;

  if (!consecutiveChord)
  {
    for (i32 i = chord.chordNotes.size() - 1; i >= 0; --i)
    {
      const Song::TranscriptionTrack::Note& note = chord.chordNotes[i];

      drawNote(note, noteTime, fretboardNoteDistance, chordBoxLeft, chordBoxRight - chordBoxLeft);
    }

    if (noteTime < 0.0f)
    {
      { // draw ChordBox
        const f32 left = Const::highwayRenderFretPosition[chordBoxLeft - 1];
        const f32 top = f32(instrumentStringCount + instrumentStringOffset) * Const::highwayRenderStringSpacing - 0.40f * Const::highwayRenderStringSpacing;
        const f32 right = Const::highwayRenderFretPosition[chordBoxRight - 1];
        const f32 bottom = -0.60f * Const::highwayRenderStringSpacing;
        const f32 posZ = noteTime * Global::settings.highwaySpeedMultiplier;

        // for sprites triangleStrip: 4 Verts + UV. Format: x,y,z,u,v
        const GLfloat v[] = {
          left , top, posZ, 0.0f, 1.0f,
          right, top, posZ, 1.0f, 1.0f,
          left, bottom, posZ, 0.0f, 0.0f,
          right, bottom, posZ, 1.0f, 0.0f,
        };

        const GLuint shader = Shader::useShader(Shader::Stem::chordBox);
        glUniform4f(glGetUniformLocation(shader, "color"), 0.2f, 0.2f, 1.0f, 0.1f);

        glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      }

      { // Draw Fret Numbers for Chord
        GLuint shader = Shader::useShader(Shader::Stem::fontWorld);
        glUniform4f(glGetUniformLocation(shader, "color"), 0.831f, 0.686f, 0.216f, 1.0f);
        for (i32 i = 1; i < 24; ++i)
        {
          if (fretsInChord & (1 << i))
          {
            const f32 x = Const::highwayRenderFretPosition[i - 1] + 0.5f * (Const::highwayRenderFretPosition[i] - Const::highwayRenderFretPosition[i - 1]);

            Font::drawFretNumber(i, x, -0.2f, noteTime * Global::settings.highwaySpeedMultiplier + 0.1f, 0.25f, 0.25f);
          }
        }
      }
    }
  }
  else
  {
    if (noteTime < 0.0f)
    { // draw consecutive ChordBox
      const f32 left = Const::highwayRenderFretPosition[chordBoxLeft - 1];
      const f32 top = 0.5f * (f32(instrumentStringCount + instrumentStringOffset) * Const::highwayRenderStringSpacing - 0.40f * Const::highwayRenderStringSpacing);
      const f32 right = Const::highwayRenderFretPosition[chordBoxRight - 1];
      const f32 bottom = -0.60f * Const::highwayRenderStringSpacing;
      const f32 posZ = noteTime * Global::settings.highwaySpeedMultiplier;

      // for sprites triangleStrip: 4 Verts + UV. Format: x,y,z,u,v
      const GLfloat v[] = {
        left , top, posZ, 0.0f, 1.0f,
        right, top, posZ, 1.0f, 1.0f,
        left, bottom, posZ, 0.0f, 0.0f,
        right, bottom, posZ, 1.0f, 0.0f,
      };

      const GLuint shader = Shader::useShader(Shader::Stem::chordBoxConsecutive);
      glUniform4f(glGetUniformLocation(shader, "color"), 0.2f, 0.2f, 1.0f, 0.1f);

      glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
  }

  drawChordName(chord.chordId, noteTime, Const::highwayRenderFretPosition[chordBoxLeft], consecutiveChord);
}

static void drawChordLeftHand(const Song::TranscriptionTrack::Chord& chord)
{
  for (const Song::TranscriptionTrack::Note& note : chord.chordNotes)
  { // Draw Left Hand (Finger Position)
    if (note.fret == 0)
      continue;

    GLuint shader = Shader::useShader(Shader::Stem::fontWorld);
    glUniform4f(glGetUniformLocation(shader, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

    const f32 x = Const::highwayRenderFretPosition[note.fret - 1] + 0.5f * (Const::highwayRenderFretPosition[note.fret] - Const::highwayRenderFretPosition[note.fret - 1]);

    Font::drawFretNumber(note.leftHand, x, f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing, 0.1f, 0.2f, 0.2f);
  }
}

static void drawChords(f32 fretboardNoteDistance[7][24])
{
  const f32 oggElapsed = Global::time - Global::oggStartTime;

  i32 leftHandNextChord = -1;
  f32 leftHandNoteTimeBegin = Const::highwayRenderLeftHandPreTime;
  //f32 leftHandNoteTimeEnd;

  for (i32 i = Global::songTrack.transcriptionTrack.chords.size() - 1; i >= 0; --i)
  {
    const Song::TranscriptionTrack::Chord& chord = Global::songTrack.transcriptionTrack.chords[i];

    const f32 noteTime = -chord.time + oggElapsed;

    f32 chordSustain = 0.0f;
    for (const Song::TranscriptionTrack::Note& note : chord.chordNotes)
    {
      chordSustain = max_(chordSustain, note.sustain);
    }

    if (noteTime - chordSustain > Const::highwayRenderDrawChordNameEndTime)
      continue;
    if (noteTime < Const::highwayRenderMaxFutureTime)
      continue;

    bool consecutiveChrod = false;
    if (i >= 1)
    {
      const Song::TranscriptionTrack::Chord& prevChord = Global::songTrack.transcriptionTrack.chords[i - 1];
      if (chord.chordNotes.size() == prevChord.chordNotes.size())
      {
        consecutiveChrod = true;
        for (i32 i = 0; i < chord.chordNotes.size(); ++i)
        {
          if (chord.chordNotes[i].string != prevChord.chordNotes[i].string || chord.chordNotes[i].fret != prevChord.chordNotes[i].fret)
          {
            consecutiveChrod = false;
            break;
          }
        }
      }
    }

    drawChord(chord, noteTime, consecutiveChrod, fretboardNoteDistance);

    if (noteTime < 0.0f && noteTime > leftHandNoteTimeBegin)
    {
      leftHandNextChord = i;
      //leftHandNoteTimeEnd = leftHandNoteTimeBegin;
      leftHandNoteTimeBegin = noteTime;
    }
  }

  if (leftHandNextChord >= 0)
  {
    const Song::TranscriptionTrack::Chord& chord = Global::songTrack.transcriptionTrack.chords[leftHandNextChord];

    drawChordLeftHand(chord);
  }
}

static void drawArpeggio(const Song::TranscriptionTrack::Note& note, f32 noteTime, i32 chordBoxLeft, i32 chordBoxWidth)
{
  const GLuint shader = Shader::useShader(Shader::Stem::defaultWorld);

  if (note.fret == 0)
  {
    setStringColor(shader, 5 - note.string + instrumentStringOffset);

    {
      mat4 modelMat;
      modelMat.m30 = Const::highwayRenderFretPosition[chordBoxLeft] + 0.05f;
      modelMat.m31 = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
      modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
      glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::zeroLeft), Data::Geometry::zeroLeft, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::zeroLeft) / (sizeof(float) * 5));

      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::arpeggioZeroLeft), Data::Geometry::arpeggioZeroLeft, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::arpeggioZeroLeft) / (sizeof(float) * 5));
    }

    {
      mat4 modelMat;
      modelMat.m00 = 16.0f;
      modelMat.m30 = Const::highwayRenderFretPosition[chordBoxLeft] + 0.266f;
      modelMat.m31 = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
      modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
      glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::zeroMiddle), Data::Geometry::zeroMiddle, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::zeroMiddle) / (sizeof(float) * 5));
    }

    {
      mat4 modelMat;
      modelMat.m30 = Const::highwayRenderFretPosition[chordBoxLeft] + 3.73f;
      modelMat.m31 = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
      modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
      glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::zeroRight), Data::Geometry::zeroRight, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::zeroRight) / (sizeof(float) * 5));

      glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::arpeggioZeroRight), Data::Geometry::arpeggioZeroRight, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::arpeggioZeroRight) / (sizeof(float) * 5));
    }

    {
      mat4 modelMat;
      modelMat.m30 = Const::highwayRenderFretPosition[chordBoxLeft] + 2.0f;
      modelMat.m31 = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
      modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
      glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
    }
  }
  else
  {
    const f32 x = Const::highwayRenderFretPosition[note.fret - 1] + 0.5f * (Const::highwayRenderFretPosition[note.fret] - Const::highwayRenderFretPosition[note.fret - 1]);

    mat4 modelMat;
    modelMat.m30 = x;
    modelMat.m31 = f32(5 - note.string + instrumentStringOffset) * Const::highwayRenderStringSpacing;
    modelMat.m32 = noteTime * Global::settings.highwaySpeedMultiplier;
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

    setStringColor(shader, 5 - note.string + instrumentStringOffset);

    glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::arpeggio), Data::Geometry::arpeggio, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::arpeggio) / (sizeof(float) * 5));
  }
}

static void drawHandShape(const Song::TranscriptionTrack::HandShape& handShape, f32 noteTimeBegin, f32 noteTimeEnd)
{
  i32 chordBoxLeft = 24;
  i32 chordBoxRight = 0;

  bool hasArpeggio = false;
  for (const Song::TranscriptionTrack::Note& note : Global::songTrack.transcriptionTrack.notes)
  {
    if (note.time >= handShape.startTime && note.time <= handShape.endTime)
    {
      if (note.fret > 0 && note.fret < chordBoxLeft)
        chordBoxLeft = note.fret;

      if (note.fret > chordBoxRight)
        chordBoxRight = note.fret;

      if (noteTimeBegin <= 0.0f)
      {
        drawArpeggio(note, noteTimeBegin, chordBoxLeft - 1, chordBoxRight - chordBoxLeft);
        hasArpeggio = true;
      }
    }
  }

  {
    i32 currentAnchor = -1;
    for (i32 i = 0; i < Global::songTrack.transcriptionTrack.anchors.size() - 2; ++i)
    {
      const Song::TranscriptionTrack::Anchor& anchor0 = Global::songTrack.transcriptionTrack.anchors[i];
      const Song::TranscriptionTrack::Anchor& anchor1 = Global::songTrack.transcriptionTrack.anchors[i + 1];

      if (handShape.startTime >= anchor0.time && handShape.endTime < anchor1.time)
      {
        currentAnchor = i;
        break;
      }
    }
    //assert(currentAnchor >= 0);
    if (currentAnchor < 0)
      currentAnchor = 0;

    if (currentAnchor >= 0)
    {
      chordBoxLeft = Global::songTrack.transcriptionTrack.anchors[currentAnchor].fret;
      chordBoxRight = Global::songTrack.transcriptionTrack.anchors[currentAnchor].fret + Global::songTrack.transcriptionTrack.anchors[currentAnchor].width;
    }
    if (chordBoxRight - chordBoxLeft < 4)
      chordBoxRight = chordBoxLeft + 4;
  }

  { // drawHandShapeAnchor

    const f32 left = Const::highwayRenderFretPosition[chordBoxLeft - 1] - 0.1f;
    const f32 right = Const::highwayRenderFretPosition[chordBoxRight - 1] + 0.1f;
    const f32 front = min_(noteTimeBegin * Global::settings.highwaySpeedMultiplier, 0.0f);
    const f32 back = noteTimeEnd * Global::settings.highwaySpeedMultiplier;


    // for sprites triangleStrip: 4 Verts + UV. Format: x,y,z,u,v

    const GLfloat v[] = {
    left , -0.35f, front, 0.0f, 1.0f,
    right, -0.35f, front, 1.0f, 1.0f,
    left, -0.35f, back, 0.0f, 0.0f,
    right, -0.35f, back, 1.0f, 0.0f,
    };

    const GLuint shader = Shader::useShader(Shader::Stem::handShapeAnchor);

    glUniform4f(glGetUniformLocation(shader, "color"), 0.588f, 0.487f, 1.0f, 0.8f);

    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    Shader::useShader(Shader::Stem::defaultWorld);
  }

  //if (hasArpeggio)
  //{ // draw ArpeggioBox
  //  const f32 left = Const::highwayRenderFretPosition[chordBoxLeft - 1];
  //  const f32 top = f32(instrumentStringCount + instrumentStringOffset) * Const::highwayRenderStringSpacing - 0.40f * Const::highwayRenderStringSpacing;
  //  const f32 right = Const::highwayRenderFretPosition[chordBoxRight - 1];
  //  const f32 bottom = -0.60f * Const::highwayRenderStringSpacing;
  //  const f32 posZ = noteTimeBegin * Global::settings.highwaySpeedMultiplier;

  //  // for sprites triangleStrip: 4 Verts + UV. Format: x,y,z,u,v
  //  const GLfloat v[] = {
  //    left , top, posZ, 0.0f, 1.0f,
  //    right, top, posZ, 1.0f, 1.0f,
  //    left, bottom, posZ, 0.0f, 0.0f,
  //    right, bottom, posZ, 1.0f, 0.0f,
  //  };

  //  Shader::useShader(Shader::Stem::chordBoxArpeggio);
  //  glUniform4f(glGetUniformLocation(shader, "color"), 0.392f, 0.325f, 0.580f, 0.1f);

  //  glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
  //  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  //  Shader::useShader(Shader::Stem::defaultWorld);

  //  drawChordName(handShape.chordId, noteTimeBegin, chordBoxLeft, false);
  //}
}

static void drawHandShapes()
{
  const f32 oggElapsed = Global::time - Global::oggStartTime;

  for (i32 i = 0; i < Global::songTrack.transcriptionTrack.handShape.size(); ++i)
  {
    const Song::TranscriptionTrack::HandShape& handShape = Global::songTrack.transcriptionTrack.handShape[i];

    const f32 noteTimeBegin = -handShape.startTime + oggElapsed;
    const f32 noteTimeEnd = -handShape.endTime + oggElapsed;

    if (noteTimeEnd > 0.0f)
      continue;
    if (noteTimeBegin < Const::highwayRenderMaxFutureTime)
      continue;

    drawHandShape(handShape, noteTimeBegin, noteTimeEnd);
  }
}

static void drawNoteFretboard(i32 fret, i32 string, f32 size)
{
  const GLuint shader = Shader::useShader(Shader::Stem::fontWorld);

  mat4 modelMat;
  modelMat.m00 = size;
  modelMat.m11 = size;
  modelMat.m22 = size;
  modelMat.m30 = Const::highwayRenderFretPosition[fret - 1] + 0.5f * (Const::highwayRenderFretPosition[fret] - Const::highwayRenderFretPosition[fret - 1]);
  modelMat.m31 = f32(string) * Const::highwayRenderStringSpacing;
  glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

  setStringColor(shader, string);

  glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::noteFretboard), Data::Geometry::noteFretboard, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::noteFretboard) / (sizeof(float) * 5));
}

static void drawNoteFreadboard(f32 fretboardNoteDistance[7][24])
{
  const f32 oggElapsed = Global::time - Global::oggStartTime;

  for (i32 i = 0; i < instrumentStringCount; ++i)
  {
    for (i32 j = 1; j < 24; ++j)
    {
      if (fretboardNoteDistance[i][j] >= 0.0f)
        continue;
      if (fretboardNoteDistance[i][j] < -1.0f)
        continue;

      f32 dist = fretboardNoteDistance[i][j];
      f32 size = 1.0f + dist;

      drawNoteFretboard(j, 5 - i + instrumentStringOffset, size);
    }
  }
}

static void drawFretNumbers()
{
  const GLuint shader = Shader::useShader(Shader::Stem::fontWorld);
  glUniform4f(glGetUniformLocation(shader, "color"), 0.831f, 0.686f, 0.216f, 1.0f);

  for (i32 i = 1; i <= 24; ++i)
  {
    const f32 x = Const::highwayRenderFretPosition[i - 1] + 0.5f * (Const::highwayRenderFretPosition[i] - Const::highwayRenderFretPosition[i - 1]);

    Font::drawFretNumber(i, x, -0.7f, 0.0f, 0.2f, 0.2f);
  }
}

static i32 getStringTuning(i32 string)
{
  const i32 psarcString = instrumentStringOffset + 5 - string;

  if (psarcString < 0 || psarcString >= 6)
    return Const::stringStandardTuningOffset[string];

  return (12 + Const::stringStandardTuningOffset[string - instrumentStringOffset] + Global::songInfos[Global::songSelected].manifest.entries[0].tuning.string[psarcString]) % 12;
}

static void drawStringNoteNames()
{
  GLuint shader = Shader::useShader(Shader::Stem::fontWorld);
  glUniform4f(glGetUniformLocation(shader, "color"), 0.5f, 0.5f, 0.5f, 0.8f);

  for (i32 y = 0; y < instrumentStringCount; ++y)
  {
    const f32 yy = f32(y) * Const::highwayRenderStringSpacing;
    const i32 stringTuning = getStringTuning(y);

    Font::drawNoteNameFlat(stringTuning, -0.2f, yy, 0.03f, 0.15f, 0.15f);
  }
}

static void drawFretNoteNames()
{
  GLuint shader = Shader::useShader(Shader::Stem::fontWorld);
  glUniform4f(glGetUniformLocation(shader, "color"), 0.5f, 0.5f, 0.5f, 0.8f);

  for (i32 y = 0; y < instrumentStringCount; ++y)
  {
    const f32 yy = f32(y) * Const::highwayRenderStringSpacing;
    const i32 stringTuning = getStringTuning(y);

    for (i32 i = 1; i <= 24; ++i)
    {
      const f32 x = Const::highwayRenderFretPosition[i - 1] + 0.2f;

      Font::drawNoteNameFlat((stringTuning + i) % 12, x, yy, 0.03f, 0.15f, 0.15f);
    }
  }
}

//static void drawCurrentChordName()
//{
//  GLuint shader = Shader::useShader(Shader::Stem::fontScreen);
//  glUniform4f(glGetUniformLocation(shader, "color"), 1.0f, 1.0f, 1.0f, 1.0f);
//
//  const Chords::Note rootNote = Global::chordDetectorRootNote;
//  Font::drawNoteNameFlat(to_underlying(rootNote), -0.2f, 0.6f, 0.0f, 0.1f, 0.1f);
//  const Chords::Quality quality = Global::chordDetectorQuality;
//  Font::drawFretNumber(to_underlying(quality), 0.0f, 0.6f, 0.0f, 0.1f, 0.1f);
//  Font::drawFretNumber(Global::chordDetectorIntervals, 0.2f, 0.6f, 0.0f, 0.1f, 0.1f);
//
//  Font::draw(Chords::chordDetectorName(), 0.0f, 0.5f, 0.0f, 0.05f, 0.05f);
//}

static void drawPhrases()
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

    f32 begin = phraseIteration0.time / Global::songInfos[Global::songSelected].manifest.entries[Global::manifestSelected].songLength;
    f32 end = phraseIteration1.time / Global::songInfos[Global::songSelected].manifest.entries[Global::manifestSelected].songLength;
    f32 difficulty = f32(phase.maxDifficulty) / f32(maxDifficulty);

    const f32 left = -0.7985 + begin * 1.6f;
    const f32 right = -0.8015 + end * 1.6f;
    const f32 posZ = 0.3f;
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

      glUniform4f(glGetUniformLocation(shader, "color"), 0.29f, 0.0f, 0.5f, 1.0f);

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

      glUniform4f(glGetUniformLocation(shader, "color"), 1.0f, 0.5f, 0.0f, 1.0f);

      glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
  }
}

static f32 leftAlign(f32 x, f32 scaleX)
{
  return x + 0.5f * scaleX;
}

static f32 rightAlign(f32 x, f32 scaleX)
{
  return x - 0.5f * scaleX;
}

static void drawSongInfo()
{
  const f32 oggElapsed = Global::time - Global::oggStartTime;

  if (Const::highwayRenderDrawSongInfoEndTime < oggElapsed)
    return;

  if (Const::highwayRenderDrawSongInfoStartTime > oggElapsed)
    return;

  f32 alpha = 1.0f;
  if (Const::highwayRenderDrawSongInfoFadeInTime > oggElapsed)
    alpha = (oggElapsed - Const::highwayRenderDrawSongInfoStartTime) / (Const::highwayRenderDrawSongInfoFadeInTime - Const::highwayRenderDrawSongInfoStartTime);
  else if (Const::highwayRenderDrawSongInfoFadeOutTime < oggElapsed)
    alpha = (oggElapsed - Const::highwayRenderDrawSongInfoEndTime) / (Const::highwayRenderDrawSongInfoFadeOutTime - Const::highwayRenderDrawSongInfoEndTime);

  GLuint shader = Shader::useShader(Shader::Stem::fontScreen);
  glUniform4f(glGetUniformLocation(shader, "color"), 1.0f, 1.0f, 1.0f, alpha);

  {
    const i32 letters = Global::songInfos[Global::songSelected].manifest.entries[Global::manifestSelected].songName.size();
    const f32 scaleX = 2.0f * f32(Const::fontCharWidth * letters) / f32(Global::settings.graphicsResolutionWidth);
    const f32 scaleY = 2.0f * f32(Const::fontCharHeight) / f32(Global::settings.graphicsResolutionHeight);
    Font::draw(Global::songInfos[Global::songSelected].manifest.entries[Global::manifestSelected].songName.c_str(), 0.95f - scaleX, 0.3f, 0.0f, scaleX, scaleY);
  }
  {
    const i32 letters = Global::songInfos[Global::songSelected].manifest.entries[Global::manifestSelected].artistName.size();
    const f32 scaleX = 2.0f * f32(Const::fontCharWidth * letters) / f32(Global::settings.graphicsResolutionWidth);
    const f32 scaleY = 2.0f * f32(Const::fontCharHeight) / f32(Global::settings.graphicsResolutionHeight);
    Font::draw(Global::songInfos[Global::songSelected].manifest.entries[Global::manifestSelected].artistName.c_str(), 0.95f - scaleX, 0.2f, 0.0f, scaleX, scaleY);
  }
}

static void drawLyricsUsedText(GLuint shader, i32& line0Cur, i32 line0Begin, i32 line0Active)
{
  if (line0Begin < line0Active)
  { // draw used text
    char line0[4096];

    i32 outLetters = 0;
    for (i32 i = line0Begin; i < line0Active; ++i)
    {
      const Song::Vocal& vocal = Global::songVocals[i];

      i32 inLetters = 0;
      while (vocal.lyric[inLetters] != '\0')
      {
        if (vocal.lyric[inLetters] != '-')
        {
          line0[outLetters] = vocal.lyric[inLetters];
          ++outLetters;
        }
        ++inLetters;
      }
      if (vocal.lyric[inLetters - 1] != '-')
      {
        line0[outLetters] = ' ';
        ++outLetters;
      }
    }

    line0Cur = outLetters;
    line0[line0Cur] = '\0';

    glUniform4f(glGetUniformLocation(shader, "color"), Global::settings.highwayLyricsColor[0].v0, Global::settings.highwayLyricsColor[0].v1, Global::settings.highwayLyricsColor[0].v2, Global::settings.highwayLyricsColor[0].v3);

    {
      const i32 letters = line0Cur;
      const f32 scaleX = 2.0f * f32(Const::fontCharWidth * letters) / f32(Global::settings.graphicsResolutionWidth);
      const f32 scaleY = 2.0f * f32(Const::fontCharHeight) / f32(Global::settings.graphicsResolutionHeight);
      Font::draw(line0, -0.95f + scaleX, 0.5f, 0.0f, scaleX, scaleY);
    }
  }
}

static bool drawLyricsActiveText(GLuint shader, i32& line0Cur, i32 line0Active)
{
  bool skipUnusedText = false;

  { // draw active text
    char line0[4096];

    i32 inLetters = 0;
    i32 outLetters = 0;
    {
      const Song::Vocal& vocal = Global::songVocals[line0Active];

      while (vocal.lyric[inLetters] != '\0')
      {
        if (vocal.lyric[inLetters] != '-')
        {
          line0[outLetters] = vocal.lyric[inLetters];
          ++outLetters;
        }
        ++inLetters;
      }
      if (vocal.lyric[inLetters - 1] == '+')
      {
        skipUnusedText = true;
        --outLetters;
      }

      line0[outLetters] = '\0';
    }

    glUniform4f(glGetUniformLocation(shader, "color"), Global::settings.highwayLyricsColor[1].v0, Global::settings.highwayLyricsColor[1].v1, Global::settings.highwayLyricsColor[1].v2, Global::settings.highwayLyricsColor[1].v3);

    {
      const f32 offsetX = 4.0f * f32(Const::fontCharWidth * line0Cur) / f32(Global::settings.graphicsResolutionWidth);
      const f32 scaleX = 2.0f * f32(Const::fontCharWidth * outLetters) / f32(Global::settings.graphicsResolutionWidth);
      const f32 scaleY = 2.0f * f32(Const::fontCharHeight) / f32(Global::settings.graphicsResolutionHeight);
      Font::draw(line0, -0.95f + scaleX + offsetX, 0.5f, 0.0f, scaleX, scaleY);
    }

    line0Cur += outLetters;
    if (Global::songVocals[line0Active].lyric[Global::songVocals[line0Active].lyric.size() - 1] != '-')
      ++line0Cur;
  }

  return skipUnusedText;
}

static void drawLyricsUnusedText(GLuint shader, i32& line0Cur, i32 line0Begin, i32 line0Active, i32 line0End)
{
  char line0[4096];

  i32 outLetters = 0;
  for (i32 i = line0Active != -1 ? line0Active + 1 : line0Begin; i < line0End; ++i)
  {
    const Song::Vocal& vocal = Global::songVocals[i];

    i32 inLetters = 0;
    while (vocal.lyric[inLetters] != '\0')
    {
      if (vocal.lyric[inLetters] != '-')
      {
        line0[outLetters] = vocal.lyric[inLetters];
        ++outLetters;
      }
      ++inLetters;
    }
    if (vocal.lyric[inLetters - 1] != '-')
    {
      line0[outLetters] = ' ';
      ++outLetters;
    }
  }
  const Song::Vocal& vocal = Global::songVocals[line0End];
  i32 j = 0;
  while (vocal.lyric[j] != '+')
  {
    line0[outLetters + j] = vocal.lyric[j];
    ++j;
  }
  line0[outLetters + j] = '\0';
  outLetters += j;

  glUniform4f(glGetUniformLocation(shader, "color"), Global::settings.highwayLyricsColor[2].v0, Global::settings.highwayLyricsColor[2].v1, Global::settings.highwayLyricsColor[2].v2, Global::settings.highwayLyricsColor[2].v3);

  {
    const f32 offsetX = 4.0f * f32(Const::fontCharWidth * line0Cur) / f32(Global::settings.graphicsResolutionWidth);
    const i32 letters = outLetters;
    const f32 scaleX = 2.0f * f32(Const::fontCharWidth * letters) / f32(Global::settings.graphicsResolutionWidth);
    const f32 scaleY = 2.0f * f32(Const::fontCharHeight) / f32(Global::settings.graphicsResolutionHeight);
    Font::draw(line0, -0.95f + scaleX + offsetX, 0.5f, 0.0f, scaleX, scaleY);
  }
}

static void drawLyricsNextLine(GLuint shader, i32 line0End)
{
  char line1[4096];
  i32 line1Cur = 0;

  i32 line1End = 0;
  for (i32 i = line0End + 1; i < Global::songVocals.size(); ++i)
  {
    const Song::Vocal& vocal = Global::songVocals[i];

    if (vocal.lyric[vocal.lyric.size() - 1] == '+')
    {
      line1End = i;
      break;
    }
  }

  for (i32 i = line0End + 1; i < line1End; ++i)
  {
    const Song::Vocal& vocal = Global::songVocals[i];

    i32 inLetters = 0;
    i32 outLetters = 0;
    while (vocal.lyric[inLetters] != '\0')
    {
      if (vocal.lyric[inLetters] != '-')
      {
        line1[line1Cur + outLetters] = vocal.lyric[inLetters];
        ++outLetters;
      }
      ++inLetters;
    }
    if (vocal.lyric[inLetters - 1] != '-')
    {
      line1[line1Cur + outLetters] = ' ';
      ++line1Cur;
    }
    line1Cur += outLetters;
  }
  const Song::Vocal& vocal = Global::songVocals[line1End];
  i32 inLetters = 0;
  i32 outLetters = 0;
  while (vocal.lyric[inLetters] != '+')
  {
    if (vocal.lyric[inLetters] != '-')
    {
      line1[line1Cur + outLetters] = vocal.lyric[inLetters];
      ++outLetters;
    }
    ++inLetters;
  }
  line1[line1Cur + outLetters] = '\0';

  glUniform4f(glGetUniformLocation(shader, "color"), Global::settings.highwayLyricsColor[2].v0, Global::settings.highwayLyricsColor[2].v1, Global::settings.highwayLyricsColor[2].v2, Global::settings.highwayLyricsColor[2].v3);
  {
    const i32 letters = line1Cur + outLetters;
    const f32 scaleX = 2.0f * f32(Const::fontCharWidth * letters) / f32(Global::settings.graphicsResolutionWidth);
    const f32 scaleY = 2.0f * f32(Const::fontCharHeight) / f32(Global::settings.graphicsResolutionHeight);
    Font::draw(line1, -0.95f + scaleX, 0.4f, 0.0f, scaleX, scaleY);
  }
}

static void drawLyrics()
{
  if (Global::songVocals.size() == 0)
    return;

  const f32 oggElapsed = Global::time - Global::oggStartTime;

  i32 line0Begin = 0;
  i32 line0Active = -1;
  i32 line0End = Global::songVocals.size() - 1;
  for (i32 i = line0Begin; i < line0End; ++i)
  {
    const Song::Vocal& vocal = Global::songVocals[i];
    const Song::Vocal& vocal2 = Global::songVocals[i + 1];

    if (vocal.time <= oggElapsed && oggElapsed < vocal2.time)
      line0Active = i;

    if (vocal.lyric[vocal.lyric.size() - 1] == '+')
    {
      if (oggElapsed < vocal2.time)
      {
        line0End = i;
        break;
      }
      line0Begin = i + 1;
    }
  }

  GLuint shader = Shader::useShader(Shader::Stem::fontScreen);
  i32 line0Cur = 0;

  bool skipUnusedText = false;
  if (line0Active != -1)
  {
    drawLyricsUsedText(shader, line0Cur, line0Begin, line0Active);
    skipUnusedText = drawLyricsActiveText(shader, line0Cur, line0Active);
  }
  if (!skipUnusedText)
  { // draw unused text
    drawLyricsUnusedText(shader, line0Cur, line0Begin, line0Active, line0End);
  }

  drawLyricsNextLine(shader, line0End);
}

static void drawEbeat(f32 noteTimeBegin, f32 noteTimeEnd, i32 measure, i32 chordBoxLeft, i32 chordBoxWidth)
{
  const f32 left = Const::highwayRenderFretPosition[chordBoxLeft - 1];
  const f32 right = Const::highwayRenderFretPosition[chordBoxLeft + chordBoxWidth - 1];
  const f32 front = noteTimeBegin * Global::settings.highwaySpeedMultiplier;
  const f32 back = noteTimeEnd * Global::settings.highwaySpeedMultiplier;

  const GLuint shader = Shader::useShader(Shader::Stem::ebeat);

  if (measure >= 0)
    glUniform4f(glGetUniformLocation(shader, "color"), Global::settings.highwayEbeatColor[0].v0, Global::settings.highwayEbeatColor[0].v1, Global::settings.highwayEbeatColor[0].v2, Global::settings.highwayEbeatColor[0].v3);
  else
    glUniform4f(glGetUniformLocation(shader, "color"), Global::settings.highwayEbeatColor[1].v0, Global::settings.highwayEbeatColor[1].v1, Global::settings.highwayEbeatColor[1].v2, Global::settings.highwayEbeatColor[1].v3);

  const GLfloat v[] = {
    left, -0.355f, front, 0.0f, 1.0f,
    right, -0.355f, front, 1.0f, 1.0f,
    left, -0.355f, back, 0.0f, 0.0f,
    right, -0.355f, back, 1.0f, 0.0f
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void drawEbeats()
{
  const f32 oggElapsed = Global::time - Global::oggStartTime;

  for (i32 i = Global::songTrack.ebeats.size() - 1; i >= 0; --i)
  {
    Song::Ebeat& ebeat = Global::songTrack.ebeats[i];

    const f32 noteTimeBegin = -ebeat.time + oggElapsed;
    const f32 noteTimeEnd = noteTimeBegin - 0.01f;

    if (noteTimeEnd > 0.0f)
      continue;
    if (noteTimeBegin < Const::highwayRenderMaxFutureTime)
      continue;

    for (i32 i = 0; i < i32(Global::songTrack.transcriptionTrack.anchors.size()) - 2; ++i)
    {
      const Song::TranscriptionTrack::Anchor& anchor0 = Global::songTrack.transcriptionTrack.anchors[i];
      const Song::TranscriptionTrack::Anchor& anchor1 = Global::songTrack.transcriptionTrack.anchors[i + 1];

      if (ebeat.time >= anchor0.time && ebeat.time <= anchor1.time)
      {
        drawEbeat(noteTimeBegin, noteTimeEnd, ebeat.measure, anchor0.fret, anchor0.width);

        break;
      }
    }
  }
}

void Highway::tick()
{
  if (Global::songInfos[Global::songSelected].loadState == Song::LoadState::complete)
  {
    if (to_underlying(Global::songInfos[Global::songSelected].manifest.entries[Global::manifestSelected].instrumentFlags & InstrumentFlags::BassGuitar))
    {
      if (Global::songInfos[Global::songSelected].manifest.entries[Global::manifestSelected].tuning.string[0] <= -3)
      {
        instrumentStringCount = 5;
        instrumentStringOffset = -1;
      }
      else
      {
        instrumentStringCount = 4;
        instrumentStringOffset = -2;
      }
      instrumentStringColors = Global::settings.instrumentBassStringColor;
      instrumentFirstWoundString = Global::settings.instrumentBassFirstWoundString;
    }
    else
    {
      if (Global::songInfos[Global::songSelected].manifest.entries[Global::manifestSelected].tuning.string[0] <= -3)
      {
        instrumentStringCount = 7;
        instrumentStringOffset = 1;
      }
      else
      {
        instrumentStringCount = 6;
        instrumentStringOffset = 0;
      }
      instrumentStringColors = Global::settings.instrumentGuitarStringColor;
      instrumentFirstWoundString = Global::settings.instrumentGuitarFirstWoundString;
    }
  }
}

void Highway::render()
{
  drawGround();
  if (Global::settings.highwayEbeat)
    drawEbeats();
  drawFrets();
  drawStrings();
  drawAnchors();
  drawHandShapes();

  f32 fretboardNoteDistance[7][24] = { };
  drawNotes(fretboardNoteDistance);
  drawChords(fretboardNoteDistance);
  drawNoteFreadboard(fretboardNoteDistance);
  drawPhrases();
  drawFretNumbers();


  if (Global::songInfos[Global::songSelected].loadState == Song::LoadState::complete && Global::settings.highwayStringNoteNames)
    drawStringNoteNames();
  if (Global::settings.highwayFretNoteNames)
    drawFretNoteNames();

  //if (Global::instrumentVolume > Const::chordDetectorVolumeThreshhold)
  //  drawCurrentChordName();

  if (Global::songInfos[Global::songSelected].loadState == Song::LoadState::complete && Global::settings.highwaySongInfo)
    drawSongInfo();

  if (Global::settings.highwayLyrics)
    drawLyrics();
}
