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
#include "font2.h"
#include "sound.h"
#include "imageload.h"

static const f32 stringSpacing = 0.42f;
static f32 highwaySpeedMultiplier = 1.0f;
static bool fretNoteNames = false;

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
  24.00f
};

static Song::TranscriptionTrack transcriptionTrack;
static std::vector<Song::Vocal> vocals;

static GLuint texture;

static i32 stringCount = 6;
static i32 stringOffset = 0;
static i32 songTuning[6];

void Highway::init()
{
  const std::vector<u8> psarcData = Psarc::readPsarcData("songs/test.psarc");

  const Psarc::PsarcInfo psarcInfo = Psarc::parse(psarcData);
  const Song::Info songInfo = Song::psarcInfoToSongInfo(psarcInfo);

  if (songInfo.tuning.string0 <= -3)
  {
    stringCount = 7;
    stringOffset = 1;
  }
  else
  {
    stringCount = 6;
    stringOffset = 0;
  }

  memcpy(songTuning, &songInfo.tuning.string0, sizeof(i32) * 6);

  auto a = Song::tuningName(songInfo.tuning);

  transcriptionTrack = Song::loadTranscriptionTrack(psarcInfo, Song::Info::InstrumentFlags::LeadGuitar);
  vocals = Song::loadVocals(psarcInfo);

  Psarc::loadOgg(psarcInfo, false);
  Sound::playOgg();

  //Global::oggStartTime = -30.0f;

  texture = loadDDS(Data::Texture::texture, sizeof(Data::Texture::texture));
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
  fretNoteNames = bool(atoi(Settings::get("Highway", "FretNoteNames").c_str()));
  tickLyrics();
}

static void setStringColor(GLuint shader, i32 string)
{
  const std::string colorStr = Settings::get("Instrument", std::string("GuitarStringColor") + std::to_string(string)).substr(1) + "FF";

  const Color color = (Color)strtoul(colorStr.c_str(), NULL, 16);

  vec4 colorVec = colorVec4(color);

  OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), colorVec.v0, colorVec.v1, colorVec.v2, colorVec.v3);
}

static void drawNote(GLuint shader, const Song::TranscriptionTrack::Note& note, f32 noteTime, f32 fretboardNoteDistance[7][24], i32 chordBoxLeft, i32 chordBoxWidth)
{
  if (noteTime > -1.0f)
  {
    if (fretboardNoteDistance[note.string][note.fret] == 0.0f || fretboardNoteDistance[note.string][note.fret] < noteTime)
      fretboardNoteDistance[note.string][note.fret] = noteTime;
  }

  if (note.fret == 0)
  {
    setStringColor(shader, 5 - note.string + stringOffset);

    {
      mat4 modelMat;
      modelMat.m30 = frets[chordBoxLeft] + frets[0] + 0.05f;
      modelMat.m31 = f32(5 - note.string + stringOffset) * stringSpacing;
      modelMat.m32 = noteTime * highwaySpeedMultiplier;
      OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

      OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::zeroLeft), Data::Geometry::zeroLeft, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::zeroLeft) / (sizeof(float) * 5));
    }

    {
      mat4 modelMat;
      modelMat.m00 = 16.0f;
      modelMat.m30 = frets[chordBoxLeft] + frets[0] + 0.266f;
      modelMat.m31 = f32(5 - note.string + stringOffset) * stringSpacing;
      modelMat.m32 = noteTime * highwaySpeedMultiplier;
      OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

      OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::zeroMiddle), Data::Geometry::zeroMiddle, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::zeroMiddle) / (sizeof(float) * 5));
    }

    {
      mat4 modelMat;
      modelMat.m30 = frets[chordBoxLeft] + frets[0] + 3.73f;
      modelMat.m31 = f32(5 - note.string + stringOffset) * stringSpacing;
      modelMat.m32 = noteTime * highwaySpeedMultiplier;
      OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

      OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::zeroRight), Data::Geometry::zeroRight, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::zeroRight) / (sizeof(float) * 5));
    }

    {
      mat4 modelMat;
      modelMat.m30 = frets[chordBoxLeft] + frets[0] + 2.0f;
      modelMat.m31 = f32(5 - note.string + stringOffset) * stringSpacing;
      modelMat.m32 = noteTime * highwaySpeedMultiplier;
      OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
    }
  }
  else
  {
    const f32 x = frets[note.fret - 1] + 0.5f * (frets[note.fret] - frets[note.fret - 1]);

    mat4 modelMat;
    modelMat.m30 = x;
    modelMat.m31 = f32(5 - note.string + stringOffset) * stringSpacing;
    modelMat.m32 = noteTime * highwaySpeedMultiplier;
    OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);

    setStringColor(shader, 5 - note.string + stringOffset);

    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::note), Data::Geometry::note, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::note) / (sizeof(float) * 5));
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
  if (note.sustain != 0.0f)
  {
    const f32 sustainTime = -note.sustain * highwaySpeedMultiplier;

    const GLfloat v[] = {
      -0.2_f32 , 0.0f, 0.0f, 0.8418f, 1.0f,
      0.2_f32, 0.0f, 0.0f, 0.9922f, 1.0f,
      -0.2_f32, 0.0f, sustainTime, 0.8418f, 0.0f,
      0.2_f32, 0.0f, sustainTime, 0.9922f, 0.0f,
    };

    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
}

static void drawNotes(GLuint shader, f32 fretboardNoteDistance[7][24])
{
  const f32 oggElapsed = Global::time - Global::oggStartTime;

  for (const Song::TranscriptionTrack::Note& note : transcriptionTrack.notes)
  {
    const f32 noteTime = -note.time + oggElapsed;

    if (noteTime > 0.0f)
      continue;
    if (noteTime < -10.0f)
      continue;

    drawNote(shader, note, noteTime, fretboardNoteDistance, 0, 4);

    if (note.fret >= 1)  // Draw Fret Numbers for Chord
    {
      GLuint shader = Shader::useShader(Shader::Stem::fontWorld);
      OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 0.831f, 0.686f, 0.216f, 1.0f);

      const f32 x = frets[note.fret - 1] + 0.5f * (frets[note.fret] - frets[note.fret - 1]);

      Font::drawFretNumber(note.fret, x, -0.03f, noteTime * highwaySpeedMultiplier, 0.5f);

      Shader::useShader(Shader::Stem::defaultWorld);
      glBindTexture(GL_TEXTURE_2D, texture);
    }
  }
}

static void drawAnchor(GLuint shader, const Song::TranscriptionTrack::Anchor& anchor, f32 noteTimeBegin, f32 noteTimeEnd)
{
  const f32 left = anchor.fret - 1;
  const f32 right = anchor.fret + anchor.width - 1;
  const f32 front = min_(noteTimeBegin * highwaySpeedMultiplier, 0.0f);
  const f32 back = noteTimeEnd * highwaySpeedMultiplier;


  // for sprites triangleStrip: 4 Verts + UV. Format: x,y,z,u,v

  const GLfloat v[] = {
  left , -0.26f, front, 0.8418f, 1.0f,
  right, -0.26f, front, 0.9922f, 1.0f,
  left, -0.26f, back, 0.8418f, 0.0f,
  right, -0.26f, back, 0.9922f, 0.0f,
  };

  Shader::useShader(Shader::Stem::anchorWorld);
  OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 0.5f, 0.1f, 0.1f, 0.4f);

  OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  Shader::useShader(Shader::Stem::defaultWorld);
}

static void drawAnchors(GLuint shader)
{
  const f32 oggElapsed = Global::time - Global::oggStartTime;

  for (i32 i = 0; i < transcriptionTrack.anchors.size() - 2; ++i)
  {
    const Song::TranscriptionTrack::Anchor& anchor0 = transcriptionTrack.anchors[i];
    const Song::TranscriptionTrack::Anchor& anchor1 = transcriptionTrack.anchors[i + 1];

    const f32 noteTimeBegin = -anchor0.time + oggElapsed;
    const f32 noteTimeEnd = -anchor1.time + oggElapsed;

    if (noteTimeEnd > 0.0f)
      continue;
    if (noteTimeBegin < -10.0f)
      continue;

    drawAnchor(shader, anchor0, noteTimeBegin, noteTimeEnd);
  }
}

static void drawChord(GLuint shader, const Song::TranscriptionTrack::Chord& chord, f32 noteTime, f32 fretboardNoteDistance[7][24])
{
  u32 fretsInChord = 0;

  i32 chordBoxLeft = 24;
  i32 chordBoxRight = 0;

  for (const Song::TranscriptionTrack::Note& note : chord.chordNotes)
  {
    if (note.fret == 0)
      chordBoxRight = chordBoxRight;

    if (note.fret > 0 && note.fret < chordBoxLeft)
      chordBoxLeft = note.fret;

    if (note.fret > chordBoxRight)
      chordBoxRight = note.fret;

    fretsInChord |= 1 << note.fret;
  }

  { // draw ChordBox
    if (chordBoxRight - chordBoxLeft < 4)
      chordBoxRight = chordBoxLeft + 4;

    const f32 left = chordBoxLeft - 1;
    const f32 top = f32(stringCount + stringOffset) * stringSpacing;
    const f32 right = chordBoxRight - 1;
    const f32 bottom = 0.0f;
    const f32 posZ = noteTime * highwaySpeedMultiplier;

    // for sprites triangleStrip: 4 Verts + UV. Format: x,y,z,u,v
    const GLfloat v[] = {
      left , top, posZ, 0.0f, 1.0f,
      right, top, posZ, 1.0f, 1.0f,
      left, bottom, posZ, 0.0f, 0.0f,
      right, bottom, posZ, 1.0f, 0.0f,
    };

    Shader::useShader(Shader::Stem::anchorWorld);
    OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 0.2f, 0.2f, 1.0f, 0.1f);

    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    Shader::useShader(Shader::Stem::defaultWorld);
  }

  for (const Song::TranscriptionTrack::Note& note : chord.chordNotes)
  {
    drawNote(shader, note, noteTime, fretboardNoteDistance, chordBoxLeft - 1, chordBoxRight - chordBoxLeft);
  }




  { // Draw Fret Numbers for Chord
    GLuint shader = Shader::useShader(Shader::Stem::fontWorld);
    OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 0.831f, 0.686f, 0.216f, 1.0f);
    for (i32 i = 1; i < 24; ++i)
    {
      if (fretsInChord & (1 << i))
      {
        const f32 x = frets[i - 1] + 0.5f * (frets[i] - frets[i - 1]);

        Font::drawFretNumber(i, x, -0.03f, noteTime * highwaySpeedMultiplier + 0.1f, 0.5f);
      }
    }

    Shader::useShader(Shader::Stem::defaultWorld);
    glBindTexture(GL_TEXTURE_2D, texture);
  }
}

static void drawChordLeftHand(const Song::TranscriptionTrack::Chord& chord)
{
  for (const Song::TranscriptionTrack::Note& note : chord.chordNotes)
  { // Draw Left Hand (Finger Position)
    if (note.fret == 0)
      continue;

    GLuint shader = Shader::useShader(Shader::Stem::fontWorld);
    OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

    const f32 x = frets[note.fret - 1] + 0.5f * (frets[note.fret] - frets[note.fret - 1]);

    Font::drawFretNumber(note.leftHand, x, f32(5 - note.string + stringOffset) * stringSpacing, 0.1f, 0.4f);

    Shader::useShader(Shader::Stem::defaultWorld);
    glBindTexture(GL_TEXTURE_2D, texture);
  }
}

static void drawChords(GLuint shader, f32 fretboardNoteDistance[7][24])
{
  const f32 oggElapsed = Global::time - Global::oggStartTime;

  i32 nextChord = -1;
  //f32 nextChordNoteTime = 0.0f;

  for (i32 i = 0; i < transcriptionTrack.chords.size(); ++i)
  {
    const Song::TranscriptionTrack::Chord& chord = transcriptionTrack.chords[i];

    const f32 noteTime = -chord.time + oggElapsed;

    if (noteTime > 0.0f)
      continue;
    if (noteTime < -10.0f)
      continue;

    drawChord(shader, chord, noteTime, fretboardNoteDistance);

    if (nextChord == -1)
    {
      //if (nextChordNoteTime == 0.0f || nextChordNoteTime > noteTime)
      {
        nextChord = i;
        //nextChordNoteTime = noteTime;
      }
    }
  }

  if (nextChord != -1)
  {
    const Song::TranscriptionTrack::Chord& chord = transcriptionTrack.chords[nextChord];

    const f32 noteTime = -chord.time + oggElapsed;

    if (noteTime > -1.0f)
    {
      drawChordLeftHand(chord);
    }
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

static void drawNoteFreadboard(GLuint shader, f32 fretboardNoteDistance[7][24])
{

  const f32 oggElapsed = Global::time - Global::oggStartTime;

  for (i32 i = 0; i < stringCount; ++i)
  {
    for (i32 j = 1; j < 24; ++j)
    {
      if (fretboardNoteDistance[i][j] >= 0.0f)
        continue;
      if (fretboardNoteDistance[i][j] < -1.0f)
        continue;

      f32 dist = fretboardNoteDistance[i][j];
      f32 size = 1.0f + dist;

      drawNoteFretboard(shader, j, 5 - i + stringOffset, size);
    }
  }
}

static void drawFretNumbers()
{
  for (i32 i = 1; i <= 24; ++i)
  {
    const f32 x = frets[i - 1] + 0.5f * (frets[i] - frets[i - 1]);

    Font::drawFretNumber(i, x, -0.7f, 0.0f, 0.4f);
  }
}

static i32 getStringTuning(i32 string)
{
  const i32 psarcString = stringOffset + 5 - string;

  if (psarcString < 0 || psarcString >= 6)
    return Const::stringStandardTuningOffset[string];

  return (12 + Const::stringStandardTuningOffset[string - stringOffset] + songTuning[psarcString]) % 12;
}

static void drawFretNoteNames()
{
  GLuint shader = Shader::useShader(Shader::Stem::fontWorld);
  OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 0.5f, 0.5f, 0.5f, 0.8f);

  for (i32 y = 0; y < stringCount; ++y)
  {
    const f32 yy = f32(y) * stringSpacing;
    const i32 stringTuning = getStringTuning(y);

    // AEADGBe

    Font::drawNoteNameFlat(stringTuning, -0.2f, yy, 0.03f, 0.3f);

    for (i32 i = 1; i <= 24; ++i)
    {
      const f32 x = frets[i - 1] + 0.2f;

      Font::drawNoteNameFlat((stringTuning + i) % 12, x, yy, 0.03f, 0.3f);
    }
  }
}

static void drawCurrentChordName()
{
  GLuint shader = Shader::useShader(Shader::Stem::fontScreen);
  OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

  const Chords::Note rootNote = Global::chordDetectorRootNote;
  Font::drawNoteNameFlat(to_underlying(rootNote), -0.2f, 0.8f, 0.0f, 0.1f);
  const Chords::Quality quality = Global::chordDetectorQuality;
  Font::drawFretNumber(to_underlying(quality), 0.0f, 0.8f, 0.0f, 0.1f);
  Font::drawFretNumber(Global::chordDetectorIntervals, 0.2f, 0.8f, 0.0f, 0.1f);

  Font::draw(Chords::name(), 0.0f, 0.7f, 0.0f, 0.1f);
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
  for (i32 i = 0; i < sizeof(frets); ++i)
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

  for (i32 i = 0; i < stringCount; ++i)
  {
    setStringColor(shader, i);
    modelMat.m31 = f32(i) * stringSpacing;
    OpenGl::glUniformMatrix4fv(OpenGl::glGetUniformLocation(shader, "model"), 1, GL_FALSE, &modelMat.m00);
    OpenGl::glBufferData(GL_ARRAY_BUFFER, sizeof(Data::Geometry::string), Data::Geometry::string, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(Data::Geometry::string) / (sizeof(float) * 5));
  }

  f32 fretboardNoteDistance[7][24] = { };

  // Draw Note
  drawAnchors(shader);
  drawNotes(shader, fretboardNoteDistance);
  drawChords(shader, fretboardNoteDistance);
  drawNoteFreadboard(shader, fretboardNoteDistance);

  shader = Shader::useShader(Shader::Stem::fontWorld);
  OpenGl::glUniform4f(OpenGl::glGetUniformLocation(shader, "color"), 0.831f, 0.686f, 0.216f, 1.0f);
  drawFretNumbers();

  if (fretNoteNames)
    drawFretNoteNames();

  drawCurrentChordName();
}
