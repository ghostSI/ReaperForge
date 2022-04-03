#include "highway.h"

#include "data.h"
#include "type.h"
#include "opengl.h"
#include "shader.h"
#include "settings.h"
#include "helper.h"
#include "global.h"
#include "psarc.h"
#include "song.h"
#include "font.h"
#include "font2.h"
#include "sound.h"
#include "imageload.h"

static const f32 stringSpacing = 0.42f;
static f32 highwaySpeedMultiplier = 1.0f;

static const f32 frets[]
{
  0.0f,
  1.00f,
  2.00f,
  3.00f,
  4.00f,
  5.00f,
  6.00f,
  7.00f,
  8.00f,
  9.00f,
  10.00f,
  11.00f,
  12.00f,
  13.00f,
  14.00f,
  15.00f,
  16.00f,
  17.00f,
  18.00f,
  19.00f,
  20.00f,
  21.00f,
  22.00f,
  23.00f,
  24.00f,
  25.00f
};

static Song::TranscriptionTrack transcriptionTrack;
static std::vector<Song::Vocal> vocals;

static GLuint texture;

void Highway::init()
{
  const std::vector<u8> psarcData = Psarc::readPsarcData("songs/test.psarc");
  const Psarc::PsarcInfo psarcInfo = Psarc::parse(psarcData);
  transcriptionTrack = Song::loadTranscriptionTrack(psarcInfo, Song::Info::InstrumentFlags::RhythmGuitar);
  vocals = Song::loadVocals(psarcInfo);

  Psarc::loadOgg(psarcInfo, false);
  Sound::playOgg();


  texture = loadDDS(Data::Texture::texture, sizeof(Data::Texture::texture));
}


static void tickFretNumbers()
{
  static Font::Handle handle[24];
  char text[3];

  for (i32 i = 0; i < 24; i += 2)
  {
    f32 posX = frets[i] + 0.5f * (frets[i + 1] - frets[i]);

    sprintf(text, "%d", i + 1);
    Font::Info fontInfo{
      .text = text,
      .fontHandle = handle[i],
      .posX = posX - 5.75f,
      .posY = -0.4f,
      .scale = 0.03f,
      .space = Space::worldSpace,
    };
    handle[i] = Font::print(fontInfo);
  }
}

static void tickLyrics()
{
  if (Settings::get("Highway", "Lyrics") == "0")
    return;

  if (vocals.size() == 0)
    return;

  const f32 oggElapsed = Global::time - Global::oggStartTime;

  i32 line0Begin = 0;
  i32 line0End = 0;
  for (i32 i = 0; i < vocals.size(); ++i)
  {
    const Song::Vocal& vocal = vocals[i];

    if (vocal.lyric[vocal.lyric.size() - 1] == '+')
    {
      if (vocal.time + vocal.length > oggElapsed)
      {
        line0End = i;
        break;
      }
      else
      {
        line0Begin = i + 1;
      }
    }
  }

  i32 line1End = 0;
  for (i32 i = line0End + 1; i < vocals.size(); ++i)
  {
    const Song::Vocal& vocal = vocals[i];

    if (vocal.lyric[vocal.lyric.size() - 1] == '+')
    {
      line1End = i;
      break;
    }
  }

  char line0[4096];
  i32 line0Cur = 0;

  for (i32 i = line0Begin; i < line0End; ++i)
  {
    const Song::Vocal& vocal = vocals[i];

    i32 j = 0;
    while (vocal.lyric[j] != '\0')
    {
      line0[line0Cur + j] = vocal.lyric[j];
      ++j;
    }
    line0[line0Cur + j] = ' ';
    line0Cur += j + 1;
  }
  const Song::Vocal& vocal = vocals[line0End];
  i32 j = 0;
  while (vocal.lyric[j] != '+')
  {
    line0[line0Cur + j] = vocal.lyric[j];
    ++j;
  }
  line0[line0Cur + j] = '\0';

  static Font::Handle line0Handle;
  Font::Info fontInfo{
    .text = line0,
    .fontHandle = line0Handle,
    .posX = 80.0f,
    .posY = 200.0f,
    .space = Space::screenSpace,
    .color = makeColor(255, 0, 0, 255)
  };
  line0Handle = Font::print(fontInfo);

  {
    char line1[4096];
    i32 line1Cur = 0;

    i32 line1End = 0;
    for (i32 i = line0End + 1; i < vocals.size(); ++i)
    {
      const Song::Vocal& vocal = vocals[i];

      if (vocal.lyric[vocal.lyric.size() - 1] == '+')
      {
        line1End = i;
        break;
      }
    }

    for (i32 i = line0End + 1; i < line1End; ++i)
    {
      const Song::Vocal& vocal = vocals[i];

      i32 j = 0;
      while (vocal.lyric[j] != '\0')
      {
        line1[line1Cur + j] = vocal.lyric[j];
        ++j;
      }
      line1[line1Cur + j] = ' ';
      line1Cur += j + 1;
    }
    const Song::Vocal& vocal = vocals[line1End];
    i32 j = 0;
    while (vocal.lyric[j] != '+')
    {
      line1[line1Cur + j] = vocal.lyric[j];
      ++j;
    }
    line1[line1Cur + j] = '\0';

    static Font::Handle line1Handle;
    Font::Info fontInfo{
      .text = line1,
      .fontHandle = line1Handle,
      .posX = 80.0f,
      .posY = 230.0f,
      .space = Space::screenSpace,
    };
    line1Handle = Font::print(fontInfo);
  }
}

void Highway::tick()
{
  highwaySpeedMultiplier = atof(Settings::get("Highway", "SpeedMultiplier").c_str());
  tickFretNumbers();
  tickLyrics();
}

static void setStringColor(GLuint shader, i32 string)
{
  const std::string colorStr = Settings::get("Instrument", std::string("GuitarStringColor") + std::to_string(string)).substr(1) + "FF";

  const Color color = (Color)strtoul(colorStr.c_str(), NULL, 16);

  vec4 colorVec = colorVec4(color);

  OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), colorVec.v0, colorVec.v1, colorVec.v2, colorVec.v3);
}

static void drawNote(GLuint shader, const Song::TranscriptionTrack::Note& note, f32 noteTime, f32 fretboardNoteDistance[6][24])
{
  if (noteTime > -1.0f)
  {
    if (fretboardNoteDistance[note.string][note.fret] == 0.0f || fretboardNoteDistance[note.string][note.fret] < noteTime)
      fretboardNoteDistance[note.string][note.fret] = noteTime;
  }

  if (note.fret == 0)
  {
    setStringColor(shader, 5 - note.string);

    {
      mat4 modelMat;
      modelMat.m30 = frets[0] + 0.05f;
      modelMat.m31 = f32(5 - note.string) * stringSpacing;
      modelMat.m32 = noteTime * highwaySpeedMultiplier;
      OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

      OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::zeroLeft), Data::Geometry::zeroLeft, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::zeroLeft) / (sizeof(float) * 5));
    }

    {
      mat4 modelMat;
      modelMat.m00 = 16.0f;
      modelMat.m30 = frets[0] + 0.266f;
      modelMat.m31 = f32(5 - note.string) * stringSpacing;
      modelMat.m32 = noteTime * highwaySpeedMultiplier;
      OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

      OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::zeroMiddle), Data::Geometry::zeroMiddle, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::zeroMiddle) / (sizeof(float) * 5));
    }

    {
      mat4 modelMat;
      modelMat.m30 = frets[0] + 3.73f;
      modelMat.m31 = f32(5 - note.string) * stringSpacing;
      modelMat.m32 = noteTime * highwaySpeedMultiplier;
      OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

      OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::zeroRight), Data::Geometry::zeroRight, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::zeroRight) / (sizeof(float) * 5));
    }
  }
  else
  {
    const f32 x = frets[note.fret - 1] + 0.5f * (frets[note.fret] - frets[note.fret - 1]);

    mat4 modelMat;
    modelMat.m30 = x;
    modelMat.m31 = f32(5 - note.string) * stringSpacing;
    modelMat.m32 = noteTime * highwaySpeedMultiplier;
    OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

    setStringColor(shader, 5 - note.string);

    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::note), Data::Geometry::note, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::note) / (sizeof(float) * 5));

    GLuint shader = Shader::useShader(Shader::Stem::fontWorld);
    OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 0.831f, 0.686f, 0.216f, 1.0f);

    char fretNumber[3];
    sprintf(fretNumber, "%d", note.fret);
    Font::draw(fretNumber, x, -0.03f, noteTime * highwaySpeedMultiplier, 0.5f);
  }


  if (note.hammerOn)
  {
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::hammerOn), Data::Geometry::hammerOn, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::hammerOn) / (sizeof(float) * 5));
  }
  if (note.harmonic)
  {
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::harmonic), Data::Geometry::harmonic, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::harmonic) / (sizeof(float) * 5));
  }
  if (note.mute)
  {
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::fretMute), Data::Geometry::fretMute, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::fretMute) / (sizeof(float) * 5));
  }
  if (note.palmMute)
  {
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::palmMute), Data::Geometry::palmMute, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::palmMute) / (sizeof(float) * 5));
  }
  if (note.pullOff)
  {
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::pullOff), Data::Geometry::pullOff, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::pullOff) / (sizeof(float) * 5));
  }
  if (note.slap)
  {
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::slap), Data::Geometry::slap, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::slap) / (sizeof(float) * 5));
  }
  if (note.harmonicPinch)
  {
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::harmonicPinch), Data::Geometry::harmonicPinch, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::harmonicPinch) / (sizeof(float) * 5));
  }
  if (note.tap)
  {
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::tap), Data::Geometry::tap, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::tap) / (sizeof(float) * 5));
  }
}

static void drawNotes(GLuint shader, f32 fretboardNoteDistance[6][24])
{
  const f32 oggElapsed = Global::time - Global::oggStartTime;

  for (const Song::TranscriptionTrack::Note& note : transcriptionTrack.notes)
  {
    const f32 noteTime = -note.time + oggElapsed;

    if (noteTime > 0.0f)
      continue;
    if (noteTime < -60.0f)
      continue;

    drawNote(shader, note, noteTime, fretboardNoteDistance);
  }
}

static void drawAnchor(GLuint shader, const Song::TranscriptionTrack::Anchor& anchor, f32 noteTime)
{
}

static void drawAnchors(GLuint shader)
{
  const f32 oggElapsed = Global::time - Global::oggStartTime;

  for (const Song::TranscriptionTrack::Anchor& ancher : transcriptionTrack.anchors)
  {
    const f32 noteTime = -ancher.time + oggElapsed;

    if (noteTime > 0.0f)
      continue;
    if (noteTime < -60.0f)
      continue;

    drawAnchor(shader, ancher, noteTime);
  }
}

static void drawChord(GLuint shader, const Song::TranscriptionTrack::Chord& chord, f32 noteTime, f32 fretboardNoteDistance[6][24])
{
  for (const Song::TranscriptionTrack::Note& note : chord.chordNotes)
  {
    drawNote(shader, note, noteTime, fretboardNoteDistance);
  }
}

static void drawChords(GLuint shader, f32 fretboardNoteDistance[6][24])
{
  const f32 oggElapsed = Global::time - Global::oggStartTime;

  for (const Song::TranscriptionTrack::Chord& chord : transcriptionTrack.chords)
  {
    const f32 noteTime = -chord.time + oggElapsed;

    if (noteTime > 0.0f)
      continue;
    if (noteTime < -60.0f)
      continue;

    drawChord(shader, chord, noteTime, fretboardNoteDistance);
  }
}

static void drawNoteFretboard(GLuint shader, i32 fret, i32 string, f32 size)
{
  mat4 modelMat;
  modelMat.m00 = size;
  modelMat.m11 = size;
  modelMat.m22 = size;
  modelMat.m30 = frets[fret - 1] + 0.5f * (frets[fret] - frets[fret - 1]);
  modelMat.m31 = f32(string) * stringSpacing;
  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

  setStringColor(shader, string);

  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::noteFretboard), Data::Geometry::noteFretboard, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::noteFretboard) / (sizeof(float) * 5));
}

static void drawNoteFreadboard(GLuint shader, f32 fretboardNoteDistance[6][24])
{

  const f32 oggElapsed = Global::time - Global::oggStartTime;

  for (i32 i = 0; i < 6; ++i)
  {
    for (i32 j = 1; j < 24; ++j)
    {
      if (fretboardNoteDistance[i][j] >= 0.0f)
        continue;
      if (fretboardNoteDistance[i][j] < -1.0f)
        continue;

      f32 dist = fretboardNoteDistance[i][j];
      f32 size = 1.0f + dist;

      drawNoteFretboard(shader, j, 5 - i, size);
    }
  }
}

void Highway::render()
{
  //GLuint shader = Shader::useShader(Shader::Stem::fontScreen);
  //OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 0.0f, 1.0f, 1.0f, 1.0f);
  //Font::draw2D("test", 0.4f, 0.4f, 0.1f);

  GLuint shader = Shader::useShader(Shader::Stem::ground);

  // Draw Ground
  {
    mat4 modelMat;
    OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::ground), Data::Geometry::ground, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::ground) / (sizeof(float) * 5));
  }

  shader = Shader::useShader(Shader::Stem::defaultWorld);

  glBindTexture(GL_TEXTURE_2D, texture);

  // Draw Fret
  OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 0.1f, 0.1f, 0.1f, 1.0f);
  for (int i = 0; i < sizeof(frets); ++i)
  {
    mat4 modelMat;
    modelMat.m30 = frets[i];
    OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::fret), Data::Geometry::fret, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::fret) / (sizeof(float) * 5));
  }

  // Draw Strings
  OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 0.2f, 0.2f, 0.2f, 1.0f);

  mat4 modelMat;
  modelMat.m00 = 600.0f;

  setStringColor(shader, 0);
  modelMat.m31 = 0.0f * stringSpacing;
  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::string), Data::Geometry::string, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::string) / (sizeof(float) * 5));

  setStringColor(shader, 1);
  modelMat.m31 = 1.0f * stringSpacing;
  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::string), Data::Geometry::string, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::string) / (sizeof(float) * 5));

  setStringColor(shader, 2);
  modelMat.m31 = 2.0f * stringSpacing;
  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::string), Data::Geometry::string, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::string) / (sizeof(float) * 5));

  setStringColor(shader, 3);
  modelMat.m31 = 3.0f * stringSpacing;
  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::string), Data::Geometry::string, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::string) / (sizeof(float) * 5));

  setStringColor(shader, 4);
  modelMat.m31 = 4.0f * stringSpacing;
  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::string), Data::Geometry::string, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::string) / (sizeof(float) * 5));

  setStringColor(shader, 5);
  modelMat.m31 = 5.0f * stringSpacing;
  OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::string), Data::Geometry::string, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::string) / (sizeof(float) * 5));


  f32 fretboardNoteDistance[6][24] = { };

  // Draw Note
  drawNotes(shader, fretboardNoteDistance);
  drawAnchors(shader);
  drawChords(shader, fretboardNoteDistance);

  drawNoteFreadboard(shader, fretboardNoteDistance);
}
