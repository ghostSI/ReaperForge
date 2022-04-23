#include "manifest.h"

#include "json.h"

Manifest::Info Manifest::readHsan(const std::vector<u8>& hsanData)
{
  Manifest::Info manifestInfo;

  Json::value_s* root = Json::parse(hsanData.data(), hsanData.size());
  assert(root->type == Json::type_object);

  Json::object_s* object = (Json::object_s*)root->payload;
  assert(object->length == 2);

  Json::object_element_s* entries = object->start;

  assert(0 == strcmp(entries->name->string, "Entries"));

  Json::value_s* entries_value = entries->value;
  assert(entries_value->type == Json::type_object);

  Json::object_s* id_o = (Json::object_s*)entries_value->payload;

  Json::object_element_s* id = id_o->start;
  do
  {
    assert(id->name->string_size == 32);

    Json::value_s* id_value = id->value;

    assert(id_value->type == Json::type_object);

    Json::object_s* attributes_o = (Json::object_s*)id_value->payload;
    assert(attributes_o->length == 1);

    {
      Manifest::Info::Attributes attrs;

      Json::object_element_s* attributes = attributes_o->start;
      assert(0 == strcmp(attributes->name->string, "Attributes"));

      {
        Json::value_s* attributes_value = attributes->value;
        assert(attributes_value->type == Json::type_object);

        Json::object_s* attribute = (Json::object_s*)attributes_value->payload;

        Json::object_element_s* it = attribute->start;
        {
          assert(0 == strcmp(it->name->string, "AlbumArt"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          attrs.albumArt = string->string;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "AlbumName"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          attrs.albumName = string->string;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "AlbumNameSort"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          attrs.albumNameSort = string->string;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "ArrangementName"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          attrs.arrangementName = string->string;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "ArtistName"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          attrs.artistName = string->string;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "ArtistNameSort"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          attrs.artistNameSort = string->string;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "CapoFret"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.capoFret = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "CentOffset"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.capoFret = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "DLC"));
          assert(it->value->type == Json::type_true || it->value->type == Json::type_false);
          attrs.dLC = it->value->type == Json::type_true;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "DLCKey"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          attrs.dLCKey = string->string;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "DNA_Chords"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.dNA_Chords = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "DNA_Riffs"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.dNA_Riffs = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "DNA_Solo"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.dNA_Solo = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "EasyMastery"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.easyMastery = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "LeaderboardChallengeRating"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.leaderboardChallengeRating = atoi(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "ManifestUrn"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          attrs.manifestUrn = string->string;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "MasterID_RDV"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.masterID_RDV = atoi(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "Metronome"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.metronome = atoi(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "MediumMastery"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.mediumMastery = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "NotesEasy"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.notesEasy = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "NotesHard"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.notesHard = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "NotesMedium"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.notesMedium = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "Representative"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.representative = atoi(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "RouteMask"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.routeMask = atoi(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "Shipping"));
          assert(it->value->type == Json::type_true || it->value->type == Json::type_false);
          attrs.dLC = it->value->type == Json::type_true;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "SKU"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          attrs.sKU = string->string;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "SongDiffEasy"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.notesMedium = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "SongDiffHard"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.notesMedium = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "SongDiffMed"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.notesMedium = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "SongDifficulty"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.notesMedium = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "SongKey"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          attrs.sKU = string->string;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "SongLength"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.songLength = atof(number->number);
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "SongName"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          attrs.songName = string->string;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "SongNameSort"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          attrs.songNameSort = string->string;
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "SongYear"));
          assert(it->value->type == Json::type_number);
          Json::number_s* number = (Json::number_s*)it->value->payload;
          attrs.songYear = atoi(number->number);
        }
        it = it->next;
        {
        }
        it = it->next;
        {
          assert(0 == strcmp(it->name->string, "PersistentID"));
          assert(it->value->type == Json::type_string);
          Json::string_s* string = (Json::string_s*)it->value->payload;
          assert(string->string_size == 32);
          attrs.persistentID = string->string;
        }



      }

      manifestInfo.attributes.push_back(attrs);
    }
  } while (id = id->next);

  Json::object_element_s* insertRoot = entries->next;
  assert(0 == strcmp(insertRoot->name->string, "InsertRoot"));
  assert(insertRoot->next == NULL);

  delete root;

  return manifestInfo;
}
