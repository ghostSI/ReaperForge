#include "chords.h"

#include "global.h"

#include <regex>

const char* Chords::chordDetectorName()
{
  if (Global::instrumentVolume < Const::chordDetectorVolumeThreshhold)
    return "";

  if (Global::chordDetectorQuality == Chords::Quality::Major && Global::chordDetectorIntervals == 0)
  {
    switch (Global::chordDetectorRootNote)
    {
    case Chords::Note::A: return "A";
    case Chords::Note::Bb:return "B\b";
    case Chords::Note::B: return "B";
    case Chords::Note::C: return "C";
    case Chords::Note::Db:return "D\b";
    case Chords::Note::D: return "D";
    case Chords::Note::Eb:return "E\b";
    case Chords::Note::E: return "E";
    case Chords::Note::F: return "F";
    case Chords::Note::Gb:return "G\b";
    case Chords::Note::G:  return "G";
    case Chords::Note::Ab: return "A\b";
    }
  }
  if (Global::chordDetectorQuality == Chords::Quality::Major && Global::chordDetectorIntervals == 7)
  {
    switch (Global::chordDetectorRootNote)
    {
    case Chords::Note::A: return "A7";
    case Chords::Note::Bb: return "B\b7";
    case Chords::Note::B: return "B7";
    case Chords::Note::C: return "C7";
    case Chords::Note::Db: return "D\b7";
    case Chords::Note::D: return "D7";
    case Chords::Note::Eb: return "E\b7";
    case Chords::Note::E: return "E7";
    case Chords::Note::F: return "F7";
    case Chords::Note::Gb: return "G\b7";
    case Chords::Note::G: return "G7";
    case Chords::Note::Ab: return "A\b7";
    }
  }
  if (Global::chordDetectorQuality == Chords::Quality::Minor && Global::chordDetectorIntervals == 0)
  {
    switch (Global::chordDetectorRootNote)
    {
    case Chords::Note::A: return "Am";
    case Chords::Note::Bb:return "B\bm";
    case Chords::Note::B: return "Bm";
    case Chords::Note::C: return "Cm";
    case Chords::Note::Db:return "D\bm";
    case Chords::Note::D: return "Dm";
    case Chords::Note::Eb:return "E\bm";
    case Chords::Note::E: return "Em";
    case Chords::Note::F: return "Fm";
    case Chords::Note::Gb:return "G\bm";
    case Chords::Note::G:  return "Gm";
    case Chords::Note::Ab: return "A\bm";
    }
  }
  if (Global::chordDetectorQuality == Chords::Quality::Minor && Global::chordDetectorIntervals == 7)
  {
    switch (Global::chordDetectorRootNote)
    {
    case Chords::Note::A: return "Am7";
    case Chords::Note::Bb:return "B\bm7";
    case Chords::Note::B: return "Bm7";
    case Chords::Note::C: return "Cm7";
    case Chords::Note::Db:return "D\bm7";
    case Chords::Note::D: return "Dm7";
    case Chords::Note::Eb:return "E\bm7";
    case Chords::Note::E: return "Em7";
    case Chords::Note::F: return "Fm7";
    case Chords::Note::Gb:return "G\bm7";
    case Chords::Note::G:  return "Gm7";
    case Chords::Note::Ab: return "A\bm7";
    }
  }
  if (Global::chordDetectorQuality == Chords::Quality::Dimished5th && Global::chordDetectorIntervals == 0)
  {
    switch (Global::chordDetectorRootNote)
    {
    case Chords::Note::A: return "Adim";
    case Chords::Note::Bb: return "B\bdim";
    case Chords::Note::B: return "Bdim";
    case Chords::Note::C: return "Cdim";
    case Chords::Note::Db: return "D\bdim";
    case Chords::Note::D: return "Ddim";
    case Chords::Note::Eb: return "E\bdim";
    case Chords::Note::E: return "Edim";
    case Chords::Note::F: return "Fdim";
    case Chords::Note::Gb: return "G\bdim";
    case Chords::Note::G: return "Gdim";
    case Chords::Note::Ab: return "A\bdim";
    }
  }
  if (Global::chordDetectorQuality == Chords::Quality::Augmented5th && Global::chordDetectorIntervals == 0)
  {
    switch (Global::chordDetectorRootNote)
    {
    case Chords::Note::A: return "Aaug";
    case Chords::Note::Bb: return "B\baug";
    case Chords::Note::B: return "Baug";
    case Chords::Note::C: return "Caug";
    case Chords::Note::Db: return "D\baug";
    case Chords::Note::D: return "Daug";
    case Chords::Note::Eb: return "E\baug";
    case Chords::Note::E: return "Eaug";
    case Chords::Note::F: return "Faug";
    case Chords::Note::Gb: return "G\baug";
    case Chords::Note::G: return "Gaug";
    case Chords::Note::Ab: return "A\baug";
    }
  }
  if (Global::chordDetectorQuality == Chords::Quality::Suspended && Global::chordDetectorIntervals == 2)
  {
    switch (Global::chordDetectorRootNote)
    {
    case Chords::Note::A: return "Asus2";
    case Chords::Note::Bb: return "B\bsus2";
    case Chords::Note::B: return "Bsus2";
    case Chords::Note::C: return "Csus2";
    case Chords::Note::Db: return "D\bsus2";
    case Chords::Note::D: return "Dsus2";
    case Chords::Note::Eb: return "E\bsus2";
    case Chords::Note::E: return "Esus2";
    case Chords::Note::F: return "Fsus2";
    case Chords::Note::Gb: return "G\bsus2";
    case Chords::Note::G: return "Gsus2";
    case Chords::Note::Ab: return "A\bsus2";
    }
  }
  if (Global::chordDetectorQuality == Chords::Quality::Suspended && Global::chordDetectorIntervals == 4)
  {
    switch (Global::chordDetectorRootNote)
    {
    case Chords::Note::A: return "Asus4";
    case Chords::Note::Bb: return "B\bsus4";
    case Chords::Note::B: return "Bsus4";
    case Chords::Note::C: return "Csus4";
    case Chords::Note::Db: return "D\bsus4";
    case Chords::Note::D: return "Dsus4";
    case Chords::Note::Eb: return "E\bsus4";
    case Chords::Note::E: return "Esus4";
    case Chords::Note::F: return "Fsus4";
    case Chords::Note::Gb: return "G\bsus4";
    case Chords::Note::G: return "Gsus4";
    case Chords::Note::Ab: return "A\bsus4";
    }
  }
  if (Global::chordDetectorQuality == Chords::Quality::Dominant && Global::chordDetectorIntervals == 7)
  {
    switch (Global::chordDetectorRootNote)
    {
    case Chords::Note::A:  return "Adom7";
    case Chords::Note::Bb: return "B\bdom7";
    case Chords::Note::B:  return "Bdom7";
    case Chords::Note::C:  return "Cdom7";
    case Chords::Note::Db: return "D\bdom7";
    case Chords::Note::D:  return "Ddom7";
    case Chords::Note::Eb: return "E\bdom7";
    case Chords::Note::E:  return "Edom7";
    case Chords::Note::F:  return "Fdom7";
    case Chords::Note::Gb: return "G\bdom7";
    case Chords::Note::G:  return "Gdom7";
    case Chords::Note::Ab: return "A\bdom7";
    }
  }

  assert(false);

  return "Unknown";
}

std::string Chords::translatedName(const std::string& name)
{
  std::string tName = name;

  if (const size_t pos = tName.find("min"); pos != std::string::npos)
    tName.replace(tName.find("min"), sizeof("min") - 1, "m");

  if (const size_t pos = tName.find("Maj"); pos != std::string::npos)
    tName.replace(tName.find("Maj"), sizeof("Maj") - 1, "");

  return tName;
}
