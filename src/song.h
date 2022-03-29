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
        std::string albumYear;
        std::string songLength;
        std::string tuning;

        i32 albumCover64_tocIndex = -1;
        mutable GLuint albumCover64_ogl = 0;
        i32 albumCover128_tocIndex = -1;
        mutable GLuint albumCover128_ogl = 0;
        i32 albumCover256_tocIndex = -1;
        mutable GLuint albumCover256_ogl = 0;
    };

    Info psarcInfoToSongInfo(const Psarc::PsarcInfo &psarcInfo);

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
        bool leftHand;
        bool mute;
        bool palmMute;
        bool pluck;
        bool pullOff;
        bool slap;
        bool slideTo;
        i32 string;
        bool sustain;
        bool tremolo;
        bool harmonicPinch;
        bool pickDirection;
        bool rightHand;
        bool slideUnpitchTo;
        bool tap;
        bool vibrato;
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
    TranscriptionTrack loadTranscriptionTrack(const Psarc::PsarcInfo& psarcInfo, Info::InstrumentFlags instrumentFlags);

    struct Vocal
    {
      f32 time;
      i32 note;
      f32 length;
      std::string lyric;
    };
    std::vector<Vocal> loadVocals(const Psarc::PsarcInfo& psarcInfo);
}

#endif // SONG_H
