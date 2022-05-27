#ifndef SONG_H
#define SONG_H

#include "arrangement.h"
#include "manifest.h"
#include "sng.h"
#include "xblock.h"

namespace Psarc { struct Info; }

namespace Song {

  enum struct LoadState
  {
    none,
    manifest,
    complete,
  };

  struct Info
  {
    LoadState loadState = LoadState::none;
    XBlock::Info xblock;
    std::vector<Manifest::Info> manifestInfos;
    std::vector<Manifest::Tone> tones;

    i32 albumCover64_tocIndex = -1;
    mutable GLuint albumCover64_ogl = 0;
    i32 albumCover128_tocIndex = -1;
    mutable GLuint albumCover128_ogl = 0;
    i32 albumCover256_tocIndex = -1;
    mutable GLuint albumCover256_ogl = 0;

    std::vector<Sng::Info> sngInfos;
#ifdef ARRANGEMENT_XML
    std::vector<Arrangement::Info> arrangements;
#endif // ARRANGEMENT_XML
  };

  Info loadSongInfoManifestOnly(const Psarc::Info& psarcInfo);
  void loadSongInfoComplete(const Psarc::Info& psarcInfo, Song::Info& songInfo);

  struct Phrase
  {
    i32 maxDifficulty;
    std::string name;
  };

  struct HeroLevel
  {
    i32 difficulty;
    i32 hero;
  };

  struct PhraseIteration
  {
    f32 time;
    i32 phraseId;
    std::string variation;
    std::vector<HeroLevel> heroLevels;
  };

  struct ChordTemplate
  {
    std::string chordName;
    std::string displayName;
    i32 finger0;
    i32 finger1;
    i32 finger2;
    i32 finger3;
    i32 finger4;
    i32 finger5;
    i32 fret0;
    i32 fret1;
    i32 fret2;
    i32 fret3;
    i32 fret4;
    i32 fret5;
  };

  struct Ebeat
  {
    f32 time;
    i32 measure;
  };

  struct Section
  {
    std::string name;
    i32 number;
    f32 startTime;
  };

  struct TranscriptionTrack
  {
    struct Note
    {
      f32 time;
      bool linkNext;
      bool accent;
      bool bend;
      i32 fret;
      bool hammerOn;
      bool harmonic;
      bool hopo;
      bool ignore;
      i32 leftHand;
      bool mute;
      bool palmMute;
      bool pluck;
      bool pullOff;
      bool slap;
      i32 slideTo;
      i32 string;
      f32 sustain;
      bool tremolo;
      bool harmonicPinch;
      bool pickDirection;
      bool rightHand;
      i32 slideUnpitchTo;
      bool tap;
      i32 vibrato;
    };
    struct Chord
    {
      f32 time;
      i32 chordId;
      bool strum;

      std::vector<Note> chordNotes;
    };
    struct Anchor
    {
      f32 time;
      i32 fret;
      i32 width;
    };
    struct HandShape
    {
      i32 chordId;
      f32 endTime;
      f32 startTime;
    };

    std::vector<Note> notes;
    std::vector<Chord> chords;
    std::vector<Anchor> anchors;
    std::vector<HandShape> handShape;
  };

  struct Track
  {
    std::vector<Phrase> phrases;
    std::vector<PhraseIteration> phraseIterations;
    std::vector<ChordTemplate> chordTemplates;
    std::vector<Ebeat> ebeats;
    std::vector<Section> sections;
    TranscriptionTrack transcriptionTrack;
  };

  Track loadTrack(const Psarc::Info& psarcInfo, InstrumentFlags instrumentFlags);

  struct Vocal
  {
    f32 time;
    i32 note;
    f32 length;
    std::string lyric;
  };
  std::vector<Vocal> loadVocals(const Psarc::Info& psarcInfo);

  const char* tuningName(const Tuning& tuning);
}

#endif // SONG_H
