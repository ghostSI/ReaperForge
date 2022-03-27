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
        int fret;
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
        bool string;
        bool sustain;
        bool tremolo;
        bool harmonicPinch;
        bool pickDirection;
        bool rightHand;
        bool slideUnpitchTo;
        bool tap;
        bool vibrato;
      };
      struct Chords
      {
        f32 time;
        u32 chordId;
        bool strum;

        struct ChordNote
        {
          f32 time;
          i32 fret;
          i32 leftHand;
          i32 string;
        };

        std::vector<ChordNote> chordNotes;
      };

      std::vector<Note> notes;
      std::vector<Chords> chords;
    };
    TranscriptionTrack loadTranscriptionTrack(const Psarc::PsarcInfo& psarcInfo, Info::InstrumentFlags instrumentFlags);
}

#endif // SONG_H
