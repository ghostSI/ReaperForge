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

static Song::Notes readSongNotes(const Psarc::PsarcInfo::TOCEntry& tocEntry) {
  Song::Notes songNotes;

  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load(reinterpret_cast<const char*>(tocEntry.content.data()));
  assert(result);

  auto a = result.description();

  pugi::xml_node notes = doc.child("song").child("transcriptionTrack").child("notes");

  //songNotes.notes.resize(notes.attribute("count").as_int());

  for (pugi::xml_node note : notes.children("note")) {

    Song::Notes::Note note_;

    note_.time = note.attribute("time").as_float();
    note_.linkNext = note.attribute("linkNext").as_bool();
    note_.accent = note.attribute("accent").as_bool();
    note_.bend = note.attribute("bend").as_bool();
    note_.fret = note.attribute("fret").as_int();
    note_.hammerOn = note.attribute("hammerOn").as_bool();
    note_.harmonic = note.attribute("harmonic").as_bool();
    note_.hopo = note.attribute("hopo").as_bool();
    note_.ignore = note.attribute("ignore").as_bool();
    note_.leftHand = note.attribute("leftHand").as_bool();
    note_.mute = note.attribute("mute").as_bool();
    note_.palmMute = note.attribute("palmMute").as_bool();
    note_.pluck = note.attribute("pluck").as_bool();
    note_.pullOff = note.attribute("pullOff").as_bool();
    note_.slap = note.attribute("slap").as_bool();
    note_.slideTo = note.attribute("slideTo").as_bool();
    note_.string = note.attribute("string ").as_bool();
    note_.sustain = note.attribute("sustain").as_bool();
    note_.tremolo = note.attribute("tremolo").as_bool();
    note_.harmonicPinch = note.attribute("harmonicPinch").as_bool();
    note_.pickDirection = note.attribute("pickDirection").as_bool();
    note_.rightHand = note.attribute("rightHand").as_bool();
    note_.slideUnpitchTo = note.attribute("slideUnpitchTo").as_bool();
    note_.tap = note.attribute("tap").as_bool();
    note_.vibrato = note.attribute("vibrato").as_bool();

    songNotes.notes.push_back(note_);
  }

  return songNotes;
}

Song::Notes Song::loadNotes(const Psarc::PsarcInfo& psarcInfo, Info::InstrumentFlags instrumentFlags)
{
  instrumentFlags = Info::InstrumentFlags::RhythmGuitar;

  for (const Psarc::PsarcInfo::TOCEntry& tocEntry : psarcInfo.tocEntries)
  {
    switch (instrumentFlags)
    {
    case Info::InstrumentFlags::LeadGuitar:
      if (tocEntry.name.ends_with("_lead.xml"))
        return readSongNotes(tocEntry);
      break;
    case Info::InstrumentFlags::RhythmGuitar:
      if (tocEntry.name.ends_with("_rhythm.xml"))
        return readSongNotes(tocEntry);
      break;
    case Info::InstrumentFlags::BassGuitar:
      if (tocEntry.name.ends_with("_bass.xml"))
        return readSongNotes(tocEntry);
      break;
    }
  }
}