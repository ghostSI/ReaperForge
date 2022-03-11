#include "song.h"

#include "psarc.h"
#include "xml.h"
#include "global.h"

static bool isXmlForInstrument(std::string filename, Instrument instrument) {

    switch (instrument) {
        case Instrument::LeadGuitar:
            if (filename.ends_with("_lead.xml"))
                return true;
            break;
        case Instrument::RhythmGuitar:
            if (filename.ends_with("_rhythm.xml"))
                return true;
            break;
        case Instrument::BassGuitar:
            if (filename.ends_with("_bass.xml"))
                return true;
            break;
    }

    return false;
}

static std::string
tuning(const std::string &string0, const std::string &string1, const std::string &string2, const std::string &string3,
       const std::string &string4, const std::string &string5) {
    if (string0 == "0" && string1 == "0" && string2 == "0" && string3 == "0" && string4 == "0" && string5 == "0")
        return "E Standard";

    assert(false);

    return "Custom Tuning";
}

static void readSongInfoXml(const Psarc::PsarcInfo::TOCEntry &tocEntry, Song::Info &songInfo) {


    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load(reinterpret_cast<const char *>(tocEntry.content.data()));
    assert(result);

    auto a = result.description();

    pugi::xml_node root = doc.child("song");

    for (pugi::xml_node node: root.child("title")) {
        songInfo.title = node.value();
    }

    for (pugi::xml_node node: root.child("tuning")) {
        songInfo.albumYear = tuning(node.attribute("string0").value(), node.attribute("string1").value(),
                                    node.attribute("string2").value(), node.attribute("string3").value(),
                                    node.attribute("string4").value(), node.attribute("string5").value());
    }

    for (pugi::xml_node node: root.child("artistName")) {
        songInfo.artist = node.value();
    }

    for (pugi::xml_node node: root.child("albumName")) {
        songInfo.albumName = node.value();
    }

    for (pugi::xml_node node: root.child("albumYear")) {
        songInfo.albumYear = node.value();
    }
}

Song::Info Song::psarcInfoToSongInfo(const Psarc::PsarcInfo &psarcInfo) {

    Song::Info songInfo;

    for (i32 i = 0; i < psarcInfo.tocEntries.size(); ++i) {
        const Psarc::PsarcInfo::TOCEntry &tocEntry = psarcInfo.tocEntries[i];

        if (tocEntry.name.ends_with("_lead.xml")) {
            songInfo.instrumentFlags |= Info::InstrumentFlags::LeadGuitar;
            readSongInfoXml(tocEntry, songInfo);
        } else if (tocEntry.name.ends_with("_rhythm.xml")) {
            songInfo.instrumentFlags |= Info::InstrumentFlags::RhythmGuitar;
            if (songInfo.title.empty())
                readSongInfoXml(tocEntry, songInfo);
        } else if (tocEntry.name.ends_with("_bass.xml")) {
            songInfo.instrumentFlags |= Info::InstrumentFlags::BassGuitar;
            if (songInfo.title.empty())
                readSongInfoXml(tocEntry, songInfo);
        } else if (tocEntry.name.ends_with("_64.dds")) {
            songInfo.albumCover64_tocIndex = i;
        } else if (tocEntry.name.ends_with("_128.dds")) {
            songInfo.albumCover128_tocIndex = i;
        } else if (tocEntry.name.ends_with("_256.dds")) {
            songInfo.albumCover256_tocIndex = i;
        }
    }

    return songInfo;
}