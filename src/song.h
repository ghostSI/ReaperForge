#ifndef SONG_H
#define SONG_H

#include "type.h"

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


}

#endif // SONG_H
