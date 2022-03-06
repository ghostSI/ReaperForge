#include "song.h"

#include "psarc.h"
#include "xml.h"

Song::Info Song::psarcInfoToSongInfo(const Psarc::PsarcInfo &psarcInfo, Instrument instrument) {
    Song::Info songInfo;

    for (const Psarc::PsarcInfo::TOCEntry &tocEntry: psarcInfo.tocEntries) {
        if (tocEntry.name == "NameBlock.bin")
            continue;

    }

    return songInfo;
}