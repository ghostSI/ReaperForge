#ifndef SONG_H
#define SONG_H

#include <string>

namespace Psarc { struct PsarcInfo; }

namespace Song {

    struct Info
    {
        std::string title;
        std::string artist;
        bool capo = false;
        std::string albumName;
        std::string albumYear;
        double songLength = 0.0;
        std::string tuning;
    };

    Info psarcInfoToSongInfo(const Psarc::PsarcInfo &psarcInfo);


}

#endif // SONG_H
