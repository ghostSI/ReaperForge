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
tuning(const std::string& string0, const std::string& string1, const std::string& string2, const std::string& string3,
  const std::string& string4, const std::string& string5) {
  if (string0 == "0" && string1 == "0" && string2 == "0" && string3 == "0" && string4 == "0" && string5 == "0")
    return "E Standard";

  assert(false);

  return "Custom Tuning";
}

static void readSongInfoXml(const Psarc::PsarcInfo::TOCEntry& tocEntry, Song::Info& songInfo) {


  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load(reinterpret_cast<const char*>(tocEntry.content.data()));
  assert(result);

  auto a = result.description();

  pugi::xml_node root = doc.child("song");

  for (pugi::xml_node node : root.child("title")) {
    songInfo.title = node.value();
  }

  for (pugi::xml_node node : root.child("tuning")) {
    songInfo.albumYear = tuning(node.attribute("string0").value(), node.attribute("string1").value(),
      node.attribute("string2").value(), node.attribute("string3").value(),
      node.attribute("string4").value(), node.attribute("string5").value());
  }

  for (pugi::xml_node node : root.child("artistName")) {
    songInfo.artist = node.value();
  }

  for (pugi::xml_node node : root.child("albumName")) {
    songInfo.albumName = node.value();
  }

  for (pugi::xml_node node : root.child("albumYear")) {
    songInfo.albumYear = node.value();
  }
}

Song::Info Song::psarcInfoToSongInfo(const Psarc::PsarcInfo& psarcInfo) {

  Song::Info songInfo;

  for (i32 i = 0; i < psarcInfo.tocEntries.size(); ++i) {
    const Psarc::PsarcInfo::TOCEntry& tocEntry = psarcInfo.tocEntries[i];

    if (tocEntry.name.ends_with("_lead.xml")) {
      songInfo.instrumentFlags |= Info::InstrumentFlags::LeadGuitar;
      readSongInfoXml(tocEntry, songInfo);
    }
    else if (tocEntry.name.ends_with("_rhythm.xml")) {
      songInfo.instrumentFlags |= Info::InstrumentFlags::RhythmGuitar;
      if (songInfo.title.empty())
        readSongInfoXml(tocEntry, songInfo);
    }
    else if (tocEntry.name.ends_with("_bass.xml")) {
      songInfo.instrumentFlags |= Info::InstrumentFlags::BassGuitar;
      if (songInfo.title.empty())
        readSongInfoXml(tocEntry, songInfo);
    }
    else if (tocEntry.name.ends_with("_64.dds")) {
      songInfo.albumCover64_tocIndex = i;
    }
    else if (tocEntry.name.ends_with("_128.dds")) {
      songInfo.albumCover128_tocIndex = i;
    }
    else if (tocEntry.name.ends_with("_256.dds")) {
      songInfo.albumCover256_tocIndex = i;
    }
  }

  return songInfo;
}

static void readSongNotes(const Psarc::PsarcInfo::TOCEntry& tocEntry, Song::TranscriptionTrack& transcriptionTrack) {
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load(reinterpret_cast<const char*>(tocEntry.content.data()));
  assert(result.status == pugi::status_ok);

  pugi::xml_node notes = doc.child("song").child("transcriptionTrack").child("notes");

  //songNotes.notes.resize(notes.attribute("count").as_int());

  for (pugi::xml_node note : notes.children("note")) {

    Song::TranscriptionTrack::Note note_;

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

    transcriptionTrack.notes.push_back(note_);
  }
}

static void readSongChords(const Psarc::PsarcInfo::TOCEntry& tocEntry, Song::TranscriptionTrack& transcriptionTrack) {
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load(reinterpret_cast<const char*>(tocEntry.content.data()));
  assert(result.status == pugi::status_ok);

  pugi::xml_node cords = doc.child("song").child("transcriptionTrack").child("chords");

  //songNotes.notes.resize(notes.attribute("count").as_int());

  for (pugi::xml_node chord : cords.children("chord")) {
    Song::TranscriptionTrack::Chord chord_;

    chord_.time = chord.attribute("time").as_float();
    chord_.chordId = chord.attribute("chordId").as_int();
    chord_.strum = chord.attribute("strum").as_bool();

    for (pugi::xml_node chordNote : chord.children("chordNote")) {

      Song::TranscriptionTrack::Chord::ChordNote chordnote_;

      chordnote_.time = chordNote.attribute("time").as_float();
      //chordnote_.linkNext = chordNote.attribute("linkNext").as_bool();
      //chordnote_.accent = chordNote.attribute("accent").as_bool();
      //chordnote_.bend = chordNote.attribute("bend").as_bool();
      chordnote_.fret = chordNote.attribute("fret").as_int();
      //chordnote_.hammerOn = chordNote.attribute("hammerOn").as_bool();
      //chordnote_.harmonic = chordNote.attribute("harmonic").as_bool();
      //chordnote_.hopo = chordNote.attribute("hopo").as_bool();
      //chordnote_.ignore = chordNote.attribute("ignore").as_bool();
      chordnote_.leftHand = chordNote.attribute("leftHand").as_bool();
      //chordnote_.mute = chordNote.attribute("mute").as_bool();
      //chordnote_.palmMute = chordNote.attribute("palmMute").as_bool();
      //chordnote_.pluck = chordNote.attribute("pluck").as_bool();
      //chordnote_.pullOff = chordNote.attribute("pullOff").as_bool();
      //chordnote_.slap = chordNote.attribute("slap").as_bool();
      //chordnote_.slideTo = chordNote.attribute("slideTo").as_bool();
      chordnote_.string = chordNote.attribute("string ").as_bool();
      //chordnote_.sustain = chordNote.attribute("sustain").as_bool();
      //chordnote_.tremolo = chordNote.attribute("tremolo").as_bool();
      //chordnote_.harmonicPinch = chordNote.attribute("harmonicPinch").as_bool();
      //chordnote_.pickDirection = chordNote.attribute("pickDirection").as_bool();
      //chordnote_.rightHand = chordNote.attribute("rightHand").as_bool();
      //chordnote_.slideUnpitchTo = chordNote.attribute("slideUnpitchTo").as_bool();
      //chordnote_.tap = chordNote.attribute("tap").as_bool();
      //chordnote_.vibrato = chordNote.attribute("vibrato").as_bool();

      chord_.chordNotes.push_back(chordnote_);
    }

    transcriptionTrack.chords.push_back(chord_);
  }
}

static void readSongAnchors(const Psarc::PsarcInfo::TOCEntry& tocEntry, Song::TranscriptionTrack& transcriptionTrack) {
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load(reinterpret_cast<const char*>(tocEntry.content.data()));
  assert(result.status == pugi::status_ok);

  pugi::xml_node anchors = doc.child("song").child("transcriptionTrack").child("anchors");

  //songNotes.notes.resize(notes.attribute("count").as_int());

  for (pugi::xml_node anchor : anchors.children("anchor")) {
    Song::TranscriptionTrack::Anchor anchor_;

    anchor_.time = anchor.attribute("time").as_float();
    anchor_.fret = anchor.attribute("fret").as_int();
    anchor_.width = anchor.attribute("width").as_int();

    transcriptionTrack.anchors.push_back(anchor_);
  }
}

static void readSongHandShape(const Psarc::PsarcInfo::TOCEntry& tocEntry, Song::TranscriptionTrack& transcriptionTrack) {
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load(reinterpret_cast<const char*>(tocEntry.content.data()));
  assert(result.status == pugi::status_ok);

  pugi::xml_node handShapes = doc.child("song").child("transcriptionTrack").child("handShapes");

  //songNotes.notes.resize(notes.attribute("count").as_int());

  for (pugi::xml_node handShape : handShapes.children("handShape")) {
    Song::TranscriptionTrack::HandShape handShape_;

    handShape_.chordId = handShape.attribute("chordId").as_int();
    handShape_.endTime = handShape.attribute("endTime").as_float();
    handShape_.startTime = handShape.attribute("startTime").as_float();

    transcriptionTrack.handShape.push_back(handShape_);
  }
}

Song::TranscriptionTrack Song::loadTranscriptionTrack(const Psarc::PsarcInfo& psarcInfo, Info::InstrumentFlags instrumentFlags)
{
  Song::TranscriptionTrack transcriptionTrack;

  for (const Psarc::PsarcInfo::TOCEntry& tocEntry : psarcInfo.tocEntries)
  {
    switch (instrumentFlags)
    {
    case Info::InstrumentFlags::LeadGuitar:
      if (tocEntry.name.ends_with("_lead.xml"))
      {
        readSongNotes(tocEntry, transcriptionTrack);
        readSongChords(tocEntry, transcriptionTrack);
        readSongAnchors(tocEntry, transcriptionTrack);
        readSongHandShape(tocEntry, transcriptionTrack);

        return transcriptionTrack;
      }
      break;
    case Info::InstrumentFlags::RhythmGuitar:
      if (tocEntry.name.ends_with("_rhythm.xml"))
      {
        readSongNotes(tocEntry, transcriptionTrack);
        readSongChords(tocEntry, transcriptionTrack);
        readSongAnchors(tocEntry, transcriptionTrack);
        readSongHandShape(tocEntry, transcriptionTrack);

        return transcriptionTrack;
      }
      break;
    case Info::InstrumentFlags::BassGuitar:
      if (tocEntry.name.ends_with("_bass.xml"))
      {
        readSongNotes(tocEntry, transcriptionTrack);
        readSongChords(tocEntry, transcriptionTrack);
        readSongAnchors(tocEntry, transcriptionTrack);
        readSongHandShape(tocEntry, transcriptionTrack);

        return transcriptionTrack;
      }
      break;
    }
  }

  return transcriptionTrack;
}