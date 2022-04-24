#include "manifest.h"

#include "json.h"
#include "xblock.h"

static void readAttribute(Json::object_element* it, Manifest::Info::Attributes& attrs)
{
  if (0 == strcmp(it->name->string, "AlbumArt"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.albumArt = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "AlbumName"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.albumName = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "AlbumNameSort"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.albumNameSort = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "ArrangementName"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.arrangementName = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "ArtistName"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.artistName = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "ArtistNameSort"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.artistNameSort = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "BassPick"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.bassPick = atoi(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "CapoFret"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.capoFret = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "CentOffset"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.capoFret = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "DLC"))
  {
    assert(it->value->type == Json::type_true || it->value->type == Json::type_false);
    attrs.dLC = it->value->type == Json::type_true;
    return;
  }
  if (0 == strcmp(it->name->string, "DLCKey"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.dLCKey = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "DNA_Chords"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.dNA_Chords = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "DNA_Riffs"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.dNA_Riffs = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "DNA_Solo"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.dNA_Solo = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "EasyMastery"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.easyMastery = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "LeaderboardChallengeRating"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.leaderboardChallengeRating = atoi(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "ManifestUrn"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.manifestUrn = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "MasterID_RDV"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.masterID_RDV = atoi(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "Metronome"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.metronome = atoi(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "MediumMastery"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.mediumMastery = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "NotesEasy"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.notesEasy = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "NotesHard"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.notesHard = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "NotesMedium"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.notesMedium = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "Representative"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.representative = atoi(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "RouteMask"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.routeMask = atoi(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "Shipping"))
  {
    assert(it->value->type == Json::type_true || it->value->type == Json::type_false);
    attrs.dLC = it->value->type == Json::type_true;
    return;
  }
  if (0 == strcmp(it->name->string, "SKU"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.sKU = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "SongDiffEasy"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.notesMedium = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "SongDiffHard"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.notesMedium = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "SongDiffMed"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.notesMedium = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "SongDifficulty"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.notesMedium = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "SongKey"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.sKU = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "SongLength"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.songLength = atof(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "SongName"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.songName = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "SongNameSort"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.songNameSort = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "SongYear"))
  {
    assert(it->value->type == Json::type_number);
    Json::number* number = (Json::number*)it->value->payload;
    attrs.songYear = atoi(number->number);
    return;
  }
  if (0 == strcmp(it->name->string, "JapaneseSongName"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.japaneseSongName = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "JapaneseArtist"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    attrs.japaneseArtist = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "Tuning"))
  {
    Json::value* tuning_value = it->value;
    assert(tuning_value->type == Json::type_object);

    Json::object* tuning_o = (Json::object*)tuning_value->payload;

    Json::object_element* string = tuning_o->start;
    {
      assert(0 == strcmp(string->name->string, "string0"));
      assert(string->value->type == Json::type_number);
      Json::number* number = (Json::number*)string->value->payload;
      attrs.tuning.string0 = atoi(number->number);
    }
    string = string->next;
    {
      assert(0 == strcmp(string->name->string, "string1"));
      assert(string->value->type == Json::type_number);
      Json::number* number = (Json::number*)string->value->payload;
      attrs.tuning.string1 = atoi(number->number);
    }
    string = string->next;
    {
      assert(0 == strcmp(string->name->string, "string2"));
      assert(string->value->type == Json::type_number);
      Json::number* number = (Json::number*)string->value->payload;
      attrs.tuning.string2 = atoi(number->number);
    }
    string = string->next;
    {
      assert(0 == strcmp(string->name->string, "string3"));
      assert(string->value->type == Json::type_number);
      Json::number* number = (Json::number*)string->value->payload;
      attrs.tuning.string3 = atoi(number->number);
    }
    string = string->next;
    {
      assert(0 == strcmp(string->name->string, "string4"));
      assert(string->value->type == Json::type_number);
      Json::number* number = (Json::number*)string->value->payload;
      attrs.tuning.string4 = atoi(number->number);
    }
    string = string->next;
    {
      assert(0 == strcmp(string->name->string, "string5"));
      assert(string->value->type == Json::type_number);
      Json::number* number = (Json::number*)string->value->payload;
      attrs.tuning.string5 = atoi(number->number);
    }
    return;
  }
  if (0 == strcmp(it->name->string, "PersistentID"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    assert(string->string_size == 32);
    attrs.persistentID = string->string;
    return;
  }

  assert(false);
}

static bool isSameId(const char* id0, const char* id1)
{
  for (i32 i = 0; i < 32; ++i)
    if (toupper(id0[i]) != toupper(id1[i]))
      return false;

  return true;
}

Manifest::Info Manifest::readHsan(const std::vector<u8>& hsanData, const XBlock::Info& xblock)
{
  Manifest::Info manifestInfo;

  Json::value* root = Json::parse(hsanData.data(), hsanData.size());
  assert(root->type == Json::type_object);

  Json::object* object = (Json::object*)root->payload;
  assert(object->length == 2);

  Json::object_element* entries = object->start;

  assert(0 == strcmp(entries->name->string, "Entries"));

  Json::value* entries_value = entries->value;
  assert(entries_value->type == Json::type_object);

  Json::object* id_o = (Json::object*)entries_value->payload;

  Json::object_element* id = id_o->start;
  do
  {
    assert(id->name->string_size == 32);

    for (const XBlock::Info::Entry& entry : xblock.entries)
    {
      if (isSameId(id->name->string, entry.id))
      {
        manifestInfo.instrumentFlags = entry.instrumentFlags;
        break;
      }
    }

    assert(manifestInfo.instrumentFlags != InstrumentFlags::none);

    Json::value* id_value = id->value;

    assert(id_value->type == Json::type_object);

    Json::object* attributes_o = (Json::object*)id_value->payload;
    assert(attributes_o->length == 1);

    {
      Manifest::Info::Attributes attrs;

      Json::object_element* attributes = attributes_o->start;
      assert(0 == strcmp(attributes->name->string, "Attributes"));

      {
        Json::value* attributes_value = attributes->value;
        assert(attributes_value->type == Json::type_object);

        Json::object* attribute = (Json::object*)attributes_value->payload;

        Json::object_element* it = attribute->start;
        do
        {
          readAttribute(it, attrs);
        } while (it = it->next);
      }

      manifestInfo.attributes.push_back(attrs);
    }
  } while (id = id->next);

  {
    Json::object_element* insertRoot = entries->next;
    assert(0 == strcmp(insertRoot->name->string, "InsertRoot"));
    assert(insertRoot->next == NULL);
    assert(insertRoot->value->type == Json::type_string);
    Json::string* string = (Json::string*)insertRoot->value->payload;
    manifestInfo.insertRoot = string->string;
  }

  delete root;

  return manifestInfo;
}
