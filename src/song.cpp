#include "song.h"

#include "psarc.h"
#include "sng.h"
#include "global.h"
#include "xml.h"

//static bool isXmlForInstrument(std::string filename, InstrumentFlags instrument) {
//
//  switch (instrument) {
//  case InstrumentFlags::LeadGuitar:
//    if (filename.ends_with("_lead.xml"))
//      return true;
//    break;
//  case InstrumentFlags::RhythmGuitar:
//    if (filename.ends_with("_rhythm.xml"))
//      return true;
//    break;
//  case InstrumentFlags::BassGuitar:
//    if (filename.ends_with("_bass.xml"))
//      return true;
//    break;
//  }
//
//  return false;
//}

Song::Info Song::loadSongInfoManifestOnly(const Psarc::Info& psarcInfo) {

  Song::Info songInfo;

  for (const Psarc::Info::TOCEntry& tocEntry : psarcInfo.tocEntries) {
    if (tocEntry.name.ends_with(".xblock"))
    {
      songInfo.xblock = XBlock::readXBlock(tocEntry.content);
      break;
    }
  }

  for (i32 i = 0; i < psarcInfo.tocEntries.size(); ++i) {
    const Psarc::Info::TOCEntry& tocEntry = psarcInfo.tocEntries[i];
    if (tocEntry.name.ends_with(".hsan"))
    {
      songInfo.manifest = Manifest::readHsan(tocEntry.content, songInfo.xblock);
      songInfo.loadState = LoadState::manifest;
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

void Song::loadSongInfoComplete(const Psarc::Info& psarcInfo, Song::Info& songInfo)
{
  assert(songInfo.loadState == LoadState::manifest);

  for (i32 i = 0; i < psarcInfo.tocEntries.size(); ++i) {
    const Psarc::Info::TOCEntry& tocEntry = psarcInfo.tocEntries[i];

    if (tocEntry.name.ends_with(".bin")
      || tocEntry.name.ends_with(".flat")
      || tocEntry.name.ends_with(".xblock")
      || tocEntry.name.ends_with(".hsan")
      || tocEntry.name.ends_with(".dds")
      || tocEntry.name.ends_with(".bnk")
      || tocEntry.name.ends_with(".wem")
      || tocEntry.name.ends_with(".nt")
      || tocEntry.name.ends_with(".sng")
      || tocEntry.name.ends_with(".version")
      || tocEntry.name.ends_with(".appid")
      || tocEntry.name.ends_with(".xml")
      || tocEntry.name.ends_with("_vocals.json"))
    {
      continue;
    }
    else if (tocEntry.name.ends_with("_lead.xml")) {
#ifdef ARRANGEMENT_XML
      Arrangement::Info arrangement = Arrangement::readArrangement(tocEntry.content);
      arrangement.instrumentFlags |= InstrumentFlags::LeadGuitar;
      songInfo.arrangements.push_back(arrangement);
#endif // ARRANGEMENT_XML
    }
    else if (tocEntry.name.ends_with("_rhythm.xml")) {
#ifdef ARRANGEMENT_XML
      Arrangement::Info arrangement = Arrangement::readArrangement(tocEntry.content);
      arrangement.instrumentFlags |= InstrumentFlags::RhythmGuitar;
      songInfo.arrangements.push_back(arrangement);
#endif // ARRANGEMENT_XML
    }
    else if (tocEntry.name.ends_with("_bass.xml")) {
#ifdef ARRANGEMENT_XML
      Arrangement::Info arrangement = Arrangement::readArrangement(tocEntry.content);
      arrangement.instrumentFlags |= InstrumentFlags::BassGuitar;
      songInfo.arrangements.push_back(arrangement);
#endif // ARRANGEMENT_XML
    }
    else if (tocEntry.name.ends_with("_lead.sng"))
    {
      const Sng::Info sngInfo = Sng::parse(tocEntry.content);
    }
    else if (tocEntry.name.ends_with("_rhythm.sng"))
    {
      songInfo.sng = Sng::parse(tocEntry.content);
    }
    else if (tocEntry.name.ends_with("_bass.sng"))
    {
      songInfo.sng = Sng::parse(tocEntry.content);
    }
    else if (tocEntry.name.ends_with("_vocals.sng"))
    {
      songInfo.sng = Sng::parse(tocEntry.content);
    }
    else if (tocEntry.name.ends_with("_lead.json"))
    {
      const std::vector<Manifest::Tone> tones = Manifest::readJson(tocEntry.content);
      songInfo.tones.insert(songInfo.tones.begin(), tones.begin(), tones.end());
    }
    else if (tocEntry.name.ends_with("_rhythm.json"))
    {
      const std::vector<Manifest::Tone> tones = Manifest::readJson(tocEntry.content);
      songInfo.tones.insert(songInfo.tones.begin(), tones.begin(), tones.end());
    }
    else if (tocEntry.name.ends_with("_rhythm2.json"))
    {
      const std::vector<Manifest::Tone> tones = Manifest::readJson(tocEntry.content);
      songInfo.tones.insert(songInfo.tones.begin(), tones.begin(), tones.end());
    }
    else if (tocEntry.name.ends_with("_bass.json"))
    {
      const std::vector<Manifest::Tone> tones = Manifest::readJson(tocEntry.content);
      songInfo.tones.insert(songInfo.tones.begin(), tones.begin(), tones.end());
    }
    else
    {
      assert(false);
    }
  }

  songInfo.loadState = LoadState::complete;
}

static void readPhrases(const pugi::xml_document& doc, std::vector<Song::Phrase>& phrases)
{
  pugi::xml_node phrases_ = doc.child("song").child("phrases");

  //phrases.resize(phrases_.attribute("count").as_int());

  for (pugi::xml_node phrase : phrases_.children("phrase")) {
    Song::Phrase phrase_;

    phrase_.maxDifficulty = phrase.attribute("maxDifficulty").as_int();
    phrase_.name = phrase.attribute("name").as_string();

    phrases.push_back(phrase_);
  }
}

static void readPhraseIterations(const pugi::xml_document& doc, std::vector<Song::PhraseIteration>& phraseIterations)
{
  pugi::xml_node phraseIterations_ = doc.child("song").child("phraseIterations");

  //phrases.resize(phrases_.attribute("count").as_int());

  for (pugi::xml_node phraseIteration : phraseIterations_.children("phraseIteration")) {
    Song::PhraseIteration phraseIteration_;

    phraseIteration_.time = phraseIteration.attribute("time").as_float();
    phraseIteration_.phraseId = phraseIteration.attribute("phraseId").as_int();
    phraseIteration_.variation = phraseIteration.attribute("variation").as_string();

    for (pugi::xml_node heroLevel : phraseIteration.child("heroLevels").children("heroLevel")) {

      Song::HeroLevel heroLevel_;

      heroLevel_.difficulty = heroLevel.attribute("difficulty").as_int();
      heroLevel_.hero = heroLevel.attribute("hero").as_int();

      phraseIteration_.heroLevels.push_back(heroLevel_);
    }

    phraseIterations.push_back(phraseIteration_);
  }
}

static void readChordTemplates(const pugi::xml_document& doc, std::vector<Song::ChordTemplate>& chordTemplates)
{
  pugi::xml_node chordTemplates_ = doc.child("song").child("chordTemplates");

  //chordTemplates.resize(chordTemplates_.attribute("count").as_int());

  for (pugi::xml_node chordTemplate : chordTemplates_.children("chordTemplate")) {
    Song::ChordTemplate chordTemplate_;

    chordTemplate_.chordName = chordTemplate.attribute("chordName").as_string();
    chordTemplate_.displayName = chordTemplate.attribute("displayName").as_string();
    chordTemplate_.finger0 = chordTemplate.attribute("finger0").as_int();
    chordTemplate_.finger1 = chordTemplate.attribute("finger1").as_int();
    chordTemplate_.finger2 = chordTemplate.attribute("finger2").as_int();
    chordTemplate_.finger3 = chordTemplate.attribute("finger3").as_int();
    chordTemplate_.finger4 = chordTemplate.attribute("finger4").as_int();
    chordTemplate_.finger5 = chordTemplate.attribute("finger5").as_int();
    chordTemplate_.fret0 = chordTemplate.attribute("fret0").as_int();
    chordTemplate_.fret1 = chordTemplate.attribute("fret1").as_int();
    chordTemplate_.fret2 = chordTemplate.attribute("fret2").as_int();
    chordTemplate_.fret3 = chordTemplate.attribute("fret3").as_int();
    chordTemplate_.fret4 = chordTemplate.attribute("fret4").as_int();
    chordTemplate_.fret5 = chordTemplate.attribute("fret5").as_int();

    chordTemplates.push_back(chordTemplate_);
  }
}

static void readEbeats(const pugi::xml_document& doc, std::vector<Song::Ebeat>& ebeats)
{
  pugi::xml_node ebeats_ = doc.child("song").child("ebeats");

  //ebeats.resize(ebeats_.attribute("count").as_int());

  for (pugi::xml_node ebeat : ebeats_.children("ebeat")) {
    Song::Ebeat ebeat_;

    ebeat_.time = ebeat.attribute("time").as_float();
    ebeat_.measure = ebeat.attribute("measure").as_int(-1);

    ebeats.push_back(ebeat_);
  }
}

static void readSections(const pugi::xml_document& doc, std::vector<Song::Section>& sections) {
  pugi::xml_node sections_ = doc.child("song").child("sections");

  //sections.resize(sections_.attribute("count").as_int());

  for (pugi::xml_node section : sections_.children("section")) {
    Song::Section section_;

    section_.name = section.attribute("name").as_string();
    section_.number = section.attribute("number").as_int();
    section_.startTime = section.attribute("startTime").as_float();

    sections.push_back(section_);
  }
}

static void readSongNotes(const pugi::xml_document& doc, std::vector<Song::TranscriptionTrack::Note>& notes) {
  pugi::xml_node notes_ = doc.child("song").child("transcriptionTrack").child("notes");

  //songNotes.notes.resize(notes.attribute("count").as_int());

  for (pugi::xml_node note : notes_.children("note")) {

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
    note_.slideTo = note.attribute("slideTo").as_int();
    note_.string = note.attribute("string").as_int();
    note_.sustain = note.attribute("sustain").as_float();
    note_.tremolo = note.attribute("tremolo").as_bool();
    note_.harmonicPinch = note.attribute("harmonicPinch").as_bool();
    note_.pickDirection = note.attribute("pickDirection").as_bool();
    note_.rightHand = note.attribute("rightHand").as_bool();
    note_.slideUnpitchTo = note.attribute("slideUnpitchTo").as_int();
    note_.tap = note.attribute("tap").as_bool();
    note_.vibrato = note.attribute("vibrato").as_int();

    notes.push_back(note_);
  }
}

static void readSongChords(const pugi::xml_document& doc, std::vector<Song::TranscriptionTrack::Chord>& chords) {
  pugi::xml_node chords_ = doc.child("song").child("transcriptionTrack").child("chords");

  //songNotes.notes.resize(notes.attribute("count").as_int());

  for (pugi::xml_node chord : chords_.children("chord")) {
    Song::TranscriptionTrack::Chord chord_;

    chord_.time = chord.attribute("time").as_float();
    chord_.chordId = chord.attribute("chordId").as_int(-1);
    chord_.strum = chord.attribute("strum").as_bool();

    for (pugi::xml_node chordNote : chord.children("chordNote")) {

      Song::TranscriptionTrack::Note note_;

      note_.time = chordNote.attribute("time").as_float();
      note_.linkNext = chordNote.attribute("linkNext").as_bool();
      note_.accent = chordNote.attribute("accent").as_bool();
      note_.bend = chordNote.attribute("bend").as_bool();
      note_.fret = chordNote.attribute("fret").as_int();
      note_.hammerOn = chordNote.attribute("hammerOn").as_bool();
      note_.harmonic = chordNote.attribute("harmonic").as_bool();
      note_.hopo = chordNote.attribute("hopo").as_bool();
      note_.ignore = chordNote.attribute("ignore").as_bool();
      note_.leftHand = chordNote.attribute("leftHand").as_int();
      note_.mute = chordNote.attribute("mute").as_bool();
      note_.palmMute = chordNote.attribute("palmMute").as_bool();
      note_.pluck = chordNote.attribute("pluck").as_bool();
      note_.pullOff = chordNote.attribute("pullOff").as_bool();
      note_.slap = chordNote.attribute("slap").as_bool();
      note_.slideTo = chordNote.attribute("slideTo").as_bool();
      note_.string = chordNote.attribute("string").as_int();
      note_.sustain = chordNote.attribute("sustain").as_float();
      note_.tremolo = chordNote.attribute("tremolo").as_bool();
      note_.harmonicPinch = chordNote.attribute("harmonicPinch").as_bool();
      note_.pickDirection = chordNote.attribute("pickDirection").as_bool();
      note_.rightHand = chordNote.attribute("rightHand").as_bool();
      note_.slideUnpitchTo = chordNote.attribute("slideUnpitchTo").as_bool();
      note_.tap = chordNote.attribute("tap").as_bool();
      note_.vibrato = chordNote.attribute("vibrato").as_bool();

      chord_.chordNotes.push_back(note_);
    }

    chords.push_back(chord_);
  }
}

static void readSongAnchors(const pugi::xml_document& doc, std::vector<Song::TranscriptionTrack::Anchor>& anchors) {
  pugi::xml_node anchors_ = doc.child("song").child("transcriptionTrack").child("anchors");

  //songNotes.notes.resize(notes.attribute("count").as_int());

  for (pugi::xml_node anchor : anchors_.children("anchor")) {
    Song::TranscriptionTrack::Anchor anchor_;

    anchor_.time = anchor.attribute("time").as_float();
    anchor_.fret = anchor.attribute("fret").as_int();
    anchor_.width = anchor.attribute("width").as_int();

    anchors.push_back(anchor_);
  }
}

static void readSongHandShape(const pugi::xml_document& doc, std::vector<Song::TranscriptionTrack::HandShape>& handShapes) {
  pugi::xml_node handShapes_ = doc.child("song").child("transcriptionTrack").child("handShapes");

  //songNotes.notes.resize(notes.attribute("count").as_int());

  for (pugi::xml_node handShape : handShapes_.children("handShape")) {
    Song::TranscriptionTrack::HandShape handShape_;

    handShape_.chordId = handShape.attribute("chordId").as_int(-1);
    handShape_.endTime = handShape.attribute("endTime").as_float();
    handShape_.startTime = handShape.attribute("startTime").as_float();

    handShapes.push_back(handShape_);
  }
}

Song::Track Song::loadTrack(const Psarc::Info& psarcInfo, InstrumentFlags instrumentFlags)
{
  Song::Track track;

  for (const Psarc::Info::TOCEntry& tocEntry : psarcInfo.tocEntries)
  {
    switch (instrumentFlags)
    {
    case InstrumentFlags::LeadGuitar:
      if (tocEntry.name.ends_with("_lead.xml"))
      {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load(reinterpret_cast<const char*>(tocEntry.content.data()));
        assert(result.status == pugi::status_ok);

        readPhrases(doc, track.phrases);
        readPhraseIterations(doc, track.phraseIterations);
        readChordTemplates(doc, track.chordTemplates);
        readEbeats(doc, track.ebeats);
        readSongNotes(doc, track.transcriptionTrack.notes);
        readSongChords(doc, track.transcriptionTrack.chords);
        readSongAnchors(doc, track.transcriptionTrack.anchors);
        readSongHandShape(doc, track.transcriptionTrack.handShape);

        return track;
      }
      break;
    case InstrumentFlags::RhythmGuitar:
      if (tocEntry.name.ends_with("_rhythm.xml"))
      {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load(reinterpret_cast<const char*>(tocEntry.content.data()));
        assert(result.status == pugi::status_ok);

        readPhrases(doc, track.phrases);
        readPhraseIterations(doc, track.phraseIterations);
        readChordTemplates(doc, track.chordTemplates);
        readEbeats(doc, track.ebeats);
        readSongNotes(doc, track.transcriptionTrack.notes);
        readSongChords(doc, track.transcriptionTrack.chords);
        readSongAnchors(doc, track.transcriptionTrack.anchors);
        readSongHandShape(doc, track.transcriptionTrack.handShape);

        return track;
      }
      break;
    case InstrumentFlags::BassGuitar:
      if (tocEntry.name.ends_with("_bass.xml"))
      {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load(reinterpret_cast<const char*>(tocEntry.content.data()));
        assert(result.status == pugi::status_ok);

        readPhrases(doc, track.phrases);
        readPhraseIterations(doc, track.phraseIterations);
        readChordTemplates(doc, track.chordTemplates);
        readEbeats(doc, track.ebeats);
        readSongNotes(doc, track.transcriptionTrack.notes);
        readSongChords(doc, track.transcriptionTrack.chords);
        readSongAnchors(doc, track.transcriptionTrack.anchors);
        readSongHandShape(doc, track.transcriptionTrack.handShape);

        return track;
      }
      break;
    }
  }

  return track;
}

std::vector<Song::Vocal> Song::loadVocals(const Psarc::Info& psarcInfo) {
  for (const Psarc::Info::TOCEntry& tocEntry : psarcInfo.tocEntries)
  {
    if (tocEntry.name.ends_with("_vocals.xml"))
    {
      pugi::xml_document doc;
      pugi::xml_parse_result result = doc.load(reinterpret_cast<const char*>(tocEntry.content.data()));
      assert(result.status == pugi::status_ok);

      pugi::xml_node vocals = doc.child("vocals");

      std::vector<Song::Vocal> vocals_;

      //songNotes.notes.resize(notes.attribute("count").as_int());

      for (pugi::xml_node vocal : vocals.children("vocal")) {
        Song::Vocal vocal_;

        vocal_.time = vocal.attribute("time").as_float();
        vocal_.note = vocal.attribute("note").as_int();
        vocal_.length = vocal.attribute("length").as_float();
        vocal_.lyric = vocal.attribute("lyric").as_string();

        vocals_.push_back(vocal_);
      }

      return vocals_;
    }
  }

  return {};
}

const char* Song::tuningName(const Tuning& tuning) {
  if (tuning.string[0] == 0 && tuning.string[1] == 0 && tuning.string[2] == 0 && tuning.string[3] == 0 && tuning.string[4] == 0 && tuning.string[5] == 0)
    return "E Standard";
  if (tuning.string[0] == -2 && tuning.string[1] == 0 && tuning.string[2] == 0 && tuning.string[3] == 0 && tuning.string[4] == 0 && tuning.string[5] == 0)
    return "Drop D";
  if (tuning.string[0] == 1 && tuning.string[1] == 1 && tuning.string[2] == 1 && tuning.string[3] == 1 && tuning.string[4] == 1 && tuning.string[5] == 1)
    return "F Standard";
  if (tuning.string[0] == -2 && tuning.string[1] == 0 && tuning.string[2] == 0 && tuning.string[3] == -1 && tuning.string[4] == -2 && tuning.string[5] == -2)
    return "Open D";
  if (tuning.string[0] == 0 && tuning.string[1] == 0 && tuning.string[2] == 2 && tuning.string[3] == 2 && tuning.string[4] == 2 && tuning.string[5] == 0)
    return "Open A";
  if (tuning.string[0] == -2 && tuning.string[1] == -2 && tuning.string[2] == 0 && tuning.string[3] == 0 && tuning.string[4] == 0 && tuning.string[5] == -2)
    return "Opend G";
  if (tuning.string[0] == 0 && tuning.string[1] == 2 && tuning.string[2] == 2 && tuning.string[3] == 1 && tuning.string[4] == 0 && tuning.string[5] == 0)
    return "Opend E";
  if (tuning.string[0] == -1 && tuning.string[1] == -1 && tuning.string[2] == -1 && tuning.string[3] == -1 && tuning.string[4] == -1 && tuning.string[5] == -1)
    return "Eb Standard";
  if (tuning.string[0] == -3 && tuning.string[1] == -1 && tuning.string[2] == -1 && tuning.string[3] == -1 && tuning.string[4] == -1 && tuning.string[5] == -1)
    return "Eb Drop Db";
  if (tuning.string[0] == -2 && tuning.string[1] == -2 && tuning.string[2] == -2 && tuning.string[3] == -2 && tuning.string[4] == -2 && tuning.string[5] == -2)
    return "D Standard";
  if (tuning.string[0] == -2 && tuning.string[1] == 0 && tuning.string[2] == 0 && tuning.string[3] == 0 && tuning.string[4] == -2 && tuning.string[5] == -2)
    return "DADGAD";
  if (tuning.string[0] == -4 && tuning.string[1] == -2 && tuning.string[2] == -2 && tuning.string[3] == -2 && tuning.string[4] == -2 && tuning.string[5] == -2)
    return "D Drop C";
  if (tuning.string[0] == -3 && tuning.string[1] == -3 && tuning.string[2] == -3 && tuning.string[3] == -3 && tuning.string[4] == -3 && tuning.string[5] == -3)
    return "C# Standard";
  if (tuning.string[0] == -5 && tuning.string[1] == -3 && tuning.string[2] == -3 && tuning.string[3] == -3 && tuning.string[4] == -3 && tuning.string[5] == -3)
    return "C# Drop B";
  if (tuning.string[0] == -4 && tuning.string[1] == -4 && tuning.string[2] == -4 && tuning.string[3] == -4 && tuning.string[4] == -4 && tuning.string[5] == -4)
    return "C Standard";
  if (tuning.string[0] == -6 && tuning.string[1] == -4 && tuning.string[2] == -4 && tuning.string[3] == -4 && tuning.string[4] == -4 && tuning.string[5] == -4)
    return "C Drap Ab";
  if (tuning.string[0] == -5 && tuning.string[1] == -5 && tuning.string[2] == -5 && tuning.string[3] == -5 && tuning.string[4] == -5 && tuning.string[5] == -5)
    return "B Standard";
  if (tuning.string[0] == -7 && tuning.string[1] == -5 && tuning.string[2] == -5 && tuning.string[3] == -5 && tuning.string[4] == -5 && tuning.string[5] == -5)
    return "B Drop A";
  if (tuning.string[0] == -6 && tuning.string[1] == -6 && tuning.string[2] == -6 && tuning.string[3] == -6 && tuning.string[4] == -6 && tuning.string[5] == -6)
    return "Bb Standard";
  if (tuning.string[0] == -8 && tuning.string[1] == -6 && tuning.string[2] == -6 && tuning.string[3] == -6 && tuning.string[4] == -6 && tuning.string[5] == -6)
    return "Bb Drop Ab";
  if (tuning.string[0] == -7 && tuning.string[1] == -7 && tuning.string[2] == -7 && tuning.string[3] == -7 && tuning.string[4] == -7 && tuning.string[5] == -7)
    return "A Standard";
  if (tuning.string[0] == -9 && tuning.string[1] == -7 && tuning.string[2] == -7 && tuning.string[3] == -7 && tuning.string[4] == -7 && tuning.string[5] == -7)
    return "A Drop G";
  if (tuning.string[0] == 0 && tuning.string[1] == 0 && tuning.string[2] == 0 && tuning.string[3] == 0 && tuning.string[4] == 1 && tuning.string[5] == 1)
    return "All Fourth";
  if (tuning.string[0] == -2 && tuning.string[1] == 0 && tuning.string[2] == 0 && tuning.string[3] == 0 && tuning.string[4] == 0 && tuning.string[5] == -2)
    return "Double Drop D";
  if (tuning.string[0] == -4 && tuning.string[1] == 0 && tuning.string[2] == -2 && tuning.string[3] == 0 && tuning.string[4] == 1 && tuning.string[5] == 0)
    return "Open C6";
  if (tuning.string[0] == -4 && tuning.string[1] == -2 && tuning.string[2] == -2 && tuning.string[3] == 0 && tuning.string[4] == -4 && tuning.string[5] == 0)
    return "Open C5";
  if (tuning.string[0] == -2 && tuning.string[1] == 0 && tuning.string[2] == 0 && tuning.string[3] == 2 && tuning.string[4] == 3 && tuning.string[5] == 2)
    return "DADADb";
  if (tuning.string[0] == -2 && tuning.string[1] == 0 && tuning.string[2] == 0 && tuning.string[3] == -2 && tuning.string[4] == 1 && tuning.string[5] == -2)
    return "Open Dm 7";
  if (tuning.string[0] == -2 && tuning.string[1] == 2 && tuning.string[2] == 0 && tuning.string[3] == -1 && tuning.string[4] == 0 && tuning.string[5] == -2)
    return "Open Bm";
  if (tuning.string[0] == 0 && tuning.string[1] == 0 && tuning.string[2] == 0 && tuning.string[3] == 0 && tuning.string[4] == 0 && tuning.string[5] == -2)
    return "EADGBD";
  if (tuning.string[0] == -2 && tuning.string[1] == 0 && tuning.string[2] == 0 && tuning.string[3] == -2 && tuning.string[4] == -2 && tuning.string[5] == -2)
    return "Open Dm";
  if (tuning.string[0] == -2 && tuning.string[1] == 2 && tuning.string[2] == 0 && tuning.string[3] == 0 && tuning.string[4] == 0 && tuning.string[5] == 0)
    return "DBDGBe";
  if (tuning.string[0] == 0 && tuning.string[1] == 0 && tuning.string[2] == 0 && tuning.string[3] == 0 && tuning.string[4] == -2 && tuning.string[5] == 0)
    return "EADGAe";
  if (tuning.string[0] == 0 && tuning.string[1] == -2 && tuning.string[2] == 0 && tuning.string[3] == 0 && tuning.string[4] == 0 && tuning.string[5] == -2)
    return "EGDGBD";
  if (tuning.string[0] == 0 && tuning.string[1] == 0 && tuning.string[2] == -3 && tuning.string[3] == 0 && tuning.string[4] == 0 && tuning.string[5] == -1)
    return "EABGBd#";
  if (tuning.string[0] == 0 && tuning.string[1] == 0 && tuning.string[2] == 0 && tuning.string[3] == 0 && tuning.string[4] == 0 && tuning.string[5] == -1)
    return "EADGBd#";
  if (tuning.string[0] == -3 && tuning.string[1] == -1 && tuning.string[2] == -1 && tuning.string[3] == -2 && tuning.string[4] == -3 && tuning.string[5] == -3)
    return "Open Db/C#";
  if (tuning.string[0] == -5 && tuning.string[1] == -5 && tuning.string[2] == -5 && tuning.string[3] == -5 && tuning.string[4] == -4 && tuning.string[5] == -5)
    return "BEADG";
  if (tuning.string[0] == -7 && tuning.string[1] == -5 && tuning.string[2] == -5 && tuning.string[3] == -5 && tuning.string[4] == -4 && tuning.string[5] == -5)
    return "AEADG";

  //assert(false);

  return "Custom Tuning";
}