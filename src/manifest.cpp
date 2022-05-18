#include "manifest.h"

#include "json.h"
#include "xblock.h"

#include <string.h>

static void readAttribute(Json::object_element* it, Manifest::Info::Entry& attrs)
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
      attrs.tuning.string[0] = atoi(number->number);
    }
    string = string->next;
    {
      assert(0 == strcmp(string->name->string, "string1"));
      assert(string->value->type == Json::type_number);
      Json::number* number = (Json::number*)string->value->payload;
      attrs.tuning.string[1] = atoi(number->number);
    }
    string = string->next;
    {
      assert(0 == strcmp(string->name->string, "string2"));
      assert(string->value->type == Json::type_number);
      Json::number* number = (Json::number*)string->value->payload;
      attrs.tuning.string[2] = atoi(number->number);
    }
    string = string->next;
    {
      assert(0 == strcmp(string->name->string, "string3"));
      assert(string->value->type == Json::type_number);
      Json::number* number = (Json::number*)string->value->payload;
      attrs.tuning.string[3] = atoi(number->number);
    }
    string = string->next;
    {
      assert(0 == strcmp(string->name->string, "string4"));
      assert(string->value->type == Json::type_number);
      Json::number* number = (Json::number*)string->value->payload;
      attrs.tuning.string[4] = atoi(number->number);
    }
    string = string->next;
    {
      assert(0 == strcmp(string->name->string, "string5"));
      assert(string->value->type == Json::type_number);
      Json::number* number = (Json::number*)string->value->payload;
      attrs.tuning.string[5] = atoi(number->number);
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

    Manifest::Info::Entry entry;

    for (const XBlock::Info::Entry& entry_ : xblock.entries)
    {
      if (isSameId(id->name->string, entry_.id))
      {
        entry.instrumentFlags = entry_.instrumentFlags;
        break;
      }
    }

    if (entry.instrumentFlags == InstrumentFlags::Vocals)
      continue;

    assert(entry.instrumentFlags != InstrumentFlags::none);

    Json::value* id_value = id->value;

    assert(id_value->type == Json::type_object);

    Json::object* attributes_o = (Json::object*)id_value->payload;
    assert(attributes_o->length == 1);

    {
      Json::object_element* attributes = attributes_o->start;
      assert(0 == strcmp(attributes->name->string, "Attributes"));

      {
        Json::value* attributes_value = attributes->value;
        assert(attributes_value->type == Json::type_object);

        Json::object* attribute = (Json::object*)attributes_value->payload;

        Json::object_element* it = attribute->start;
        do
        {
          readAttribute(it, entry);
        } while (it = it->next);
      }

      manifestInfo.entries.push_back(entry);
    }
  } while (id = id->next);

  delete root;

  return manifestInfo;
}

static void readGear(Json::object_element* it, Manifest::Tone::GearList::Gear& gear)
{
  Json::value* entries_value = it->value;
  assert(entries_value->type == Json::type_object);


  Json::object* id_o = (Json::object*)entries_value->payload;
  assert(id_o->length >= 3);

  Json::object_element* it2 = id_o->start;

  {
    assert(0 == strcmp(it2->name->string, "Type"));
    Json::string* typeValue = (Json::string*)it2->value->payload;
    gear.type = typeValue->string;
  }
  it2 = it2->next;
  {
    assert(0 == strcmp(it2->name->string, "KnobValues"));

    Json::value* entries_value = it2->value;
    assert(entries_value->type == Json::type_object);

    Json::object* id_o = (Json::object*)entries_value->payload;

    if (id_o->length >= 1)
    {
      Json::object_element* it = id_o->start;
      do
      {
        Json::number* value = (Json::number*)it->value->payload;
        Manifest::Tone::GearList::Gear::KnobValue knobValue;
        knobValue.name = it->name->string;
        knobValue.value = atof(value->number);
        gear.knobValues.push_back(knobValue);
      } while (it = it->next);
    }
  }
  it2 = it2->next;
  {
    assert(0 == strcmp(it2->name->string, "Key"));
    Json::string* keyValue = (Json::string*)it2->value->payload;
    gear.key = keyValue->string;
  }

  if (id_o->length == 4)
  {
    it2 = it2->next;
    assert(0 == strcmp(it2->name->string, "Category"));
    Json::string* keyValue = (Json::string*)it2->value->payload;
    gear.category = keyValue->string;
  }

}

static void readGearList(Json::object_element* it, Manifest::Tone::GearList& gearList)
{
  do
  {
    assert(it->value->type == Json::type_object);

    if (0 == strcmp(it->name->string, "Rack1"))
      readGear(it, gearList.rack1);
    else if (0 == strcmp(it->name->string, "Rack2"))
      readGear(it, gearList.rack2);
    else if (0 == strcmp(it->name->string, "Rack3"))
      readGear(it, gearList.rack3);
    else if (0 == strcmp(it->name->string, "Amp"))
      readGear(it, gearList.amp);
    else if (0 == strcmp(it->name->string, "Cabinet"))
      readGear(it, gearList.cabinet);
    else if (0 == strcmp(it->name->string, "PrePedal1"))
      readGear(it, gearList.prePedal1);
    else if (0 == strcmp(it->name->string, "PrePedal2"))
      readGear(it, gearList.prePedal2);
    else if (0 == strcmp(it->name->string, "PrePedal3"))
      readGear(it, gearList.prePedal3);
    else if (0 == strcmp(it->name->string, "PostPedal1"))
      readGear(it, gearList.postPedal1);
    else
      assert(false);
  } while (it = it->next);
}

static void readTone(Json::object_element* it, Manifest::Tone& tone)
{
  if (0 == strcmp(it->name->string, "GearList"))
  {
    Json::value* attribute = it->value;
    assert(attribute->type == Json::type_object);

    Json::object* arr = (Json::object*)attribute->payload;

    Json::object_element* it = arr->start;

    do
    {
      readGearList(it, tone.gearList);
    } while (it = it->next);
    return;
  }
  if (0 == strcmp(it->name->string, "IsCustom"))
  {
    assert(it->value->type == Json::type_true || it->value->type == Json::type_false);
    tone.isCustom = it->value->type == Json::type_true;
    return;
  }
  if (0 == strcmp(it->name->string, "Volume"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    tone.volume = atof(string->string);
    return;
  }
  if (0 == strcmp(it->name->string, "ToneDescriptors"))
  {
    assert(it->value->type == Json::type_array);
    Json::array* arr = (Json::array*)it->value->payload;
    assert(arr->length == 1);
    Json::array_element* it2 = arr->start;
    Json::value* arrValue = it2->value;
    assert(arrValue->type == Json::type_string);
    Json::string* string = (Json::string*)arrValue->payload;
    tone.toneDescriptors.push_back(string->string);
    return;
  }
  if (0 == strcmp(it->name->string, "Key"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    tone.key = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "NameSeparator"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    tone.nameSeparator = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "Name"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    tone.name = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "SortOrder"))
  {
    assert(it->value->type == Json::type_number);
    Json::string* string = (Json::string*)it->value->payload;
    tone.sortOrder = atof(string->string);
    return;
  }
}

std::vector<Manifest::Tone> Manifest::readJson(const std::vector<u8>& jsonData)
{
  Json::value* root = Json::parse(jsonData.data(), jsonData.size());
  assert(root->type == Json::type_object);

  Json::object* object = (Json::object*)root->payload;
  assert(object->length == 4);

  Json::object_element* entries = object->start;

  assert(0 == strcmp(entries->name->string, "Entries"));

  Json::value* entries_value = entries->value;
  assert(entries_value->type == Json::type_object);

  Json::object* id_o = (Json::object*)entries_value->payload;
  assert(id_o->length == 1);

  Json::object_element* persistentId = id_o->start;

  //assert(0 == strcmp(persistentId->name->string, "887710005D5C4A42A788F2BBA5FD9A41"));

  Json::value* persistentId_value = persistentId->value;
  assert(persistentId_value->type == Json::type_object);

  Json::object* attributes = (Json::object*)persistentId_value->payload;
  assert(attributes->length == 1);

  Json::object_element* attr = attributes->start;

  assert(0 == strcmp(attr->name->string, "Attributes"));

  Json::value* attribute = attr->value;
  assert(attribute->type == Json::type_object);

  Json::object* arr = (Json::object*)attribute->payload;

  Json::object_element* it = arr->start;


  std::vector<Manifest::Tone> tones;
  do
  {
    if (0 == strcmp(it->name->string, "Tones"))
    {
      Json::value* itValue = it->value;

      assert(itValue->type == Json::type_array);

      Json::array* arr = (Json::array*)itValue->payload;

      Json::array_element* it2 = arr->start;

      Json::value* arrValue = it2->value;

      Json::object* obj = (Json::object*)arrValue->payload;

      Json::object_element* it3 = obj->start;

      Tone tone;
      do
      {
        readTone(it3, tone);
      } while (it3 = it3->next);
      tones.push_back(tone);
    }
  } while (it = it->next);

  return tones;
}
