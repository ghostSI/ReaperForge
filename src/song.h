#ifndef SONG_H
#define SONG_H

#include "type.h"

#include <vector>

namespace Psarc { struct PsarcInfo; }

namespace Song {

    struct Info
    {
        enum InstrumentFlags
        {
            none = 0,
            LeadGuitar = 1,
            RhythmGuitar = 1 << 1,
            BassGuitar = 1 << 2,
        }BIT_FLAGS_FRIEND(Info::InstrumentFlags);

        InstrumentFlags instrumentFlags = InstrumentFlags::none;
        std::string title;
        std::string artist;
        bool capo = false;
        std::string albumName;
        i32 albumYear;
        f32 songLength;

        struct Tuning
        {
          i32 string0;
          i32 string1;
          i32 string2;
          i32 string3;
          i32 string4;
          i32 string5;
        } tuning;

        struct ArrangementProperties
        {
          bool represent;
          bool standardTuning;
          bool nonStandardChords;
          bool barreChords;
          bool powerChords;
          bool dropDPower;
          bool openChords;
          bool fingerPicking;
          bool pickDirection;
          bool doubleStops;
          bool palmMutes;
          bool harmonics;
          bool pinchHarmonics;
          bool hopo;
          bool tremolo;
          bool slides;
          bool unpitchedSlides;
          bool bends;
          bool tapping;
          bool vibrato;
          bool fretHandMutes;
          bool slapPop;
          bool twoFingerPicking;
          bool fifthsAndOctaves;
          bool syncopation;
          bool bassPick;
          bool sustain; 
          bool bonusArr;
          bool Metronome;
          bool pathLead;
          bool pathRhythm;
          bool pathBass;
          bool routeMask;
        } arrangementProperties;

        i32 albumCover64_tocIndex = -1;
        mutable GLuint albumCover64_ogl = 0;
        i32 albumCover128_tocIndex = -1;
        mutable GLuint albumCover128_ogl = 0;
        i32 albumCover256_tocIndex = -1;
        mutable GLuint albumCover256_ogl = 0;
    };

    Info psarcInfoToSongInfo(const Psarc::PsarcInfo &psarcInfo);

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

    Track loadTrack(const Psarc::PsarcInfo& psarcInfo, Info::InstrumentFlags instrumentFlags);

    struct Vocal
    {
      f32 time;
      i32 note;
      f32 length;
      std::string lyric;
    };
    std::vector<Vocal> loadVocals(const Psarc::PsarcInfo& psarcInfo);

    const char* tuningName(const Song::Info::Tuning& tuning);
}

#endif // SONG_H
