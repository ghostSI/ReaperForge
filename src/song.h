#ifndef SONG_H
#define SONG_H

#include "type.h"

namespace Psarc { struct PsarcInfo; }

namespace Song {

    struct Info
    {
        std::string title;
        std::string artist;
        bool capo = false;
        std::string albumName;
        double songLength = 0.0;
    };

    Info psarcInfoToSongInfo(const Psarc::PsarcInfo &psarcInfo, Instrument instrument);
}

#endif // SONG_H
