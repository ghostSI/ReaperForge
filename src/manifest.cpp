#include "manifest.h"

#include "json.h"

Manifest::Info Manifest::readHsan(const std::vector<u8>& hsanData)
{
  Manifest::Info manifestInfo;
  manifestInfo.entries.resize(1);

  Json::value_s* root = Json::parse(hsanData.data(), hsanData.size());
  assert(root->type == Json::type_object);

  Json::object_s* object = (Json::object_s*)root->payload;
  assert(object->length == 2);

  Json::object_element_s* entries = object->start;
  assert(0 == strcmp(entries->name->string, "Entries"));

  {
    Json::value_s* entries_value = entries->value;
    assert(entries_value->type == Json::type_object);

    Json::object_s* id_o = (Json::object_s*)entries_value->payload;
    Json::object_element_s* id = id_o->start;
    assert(id->name->string_size == 32);

    Json::value_s* id_value = id->value;
    assert(id_value->type == Json::type_object);

    Json::object_s* attributes_o = (Json::object_s*)id_value->payload;
    assert(attributes_o->length == 1);

    Json::object_element_s* attributes = attributes_o->start;
    assert(0 == strcmp(attributes->name->string, "Attributes"));

    {
      Json::value_s* attributes_value = attributes->value;
      assert(attributes_value->type == Json::type_object);

      Json::object_s* albumArt_o = (Json::object_s*)attributes_value->payload;
      Json::object_element_s* albumArt = albumArt_o->start;
      assert(0 == strcmp(albumArt->name->string, "AlbumArt"));
      assert(albumArt->value->type == Json::type_string);
      Json::string_s* albumArt_string = (Json::string_s*)albumArt->value->payload;
      manifestInfo.entries[0].albumArt = albumArt_string->string;
    }

  }

  Json::object_element_s* insertRoot = entries->next;
  assert(0 == strcmp(insertRoot->name->string, "InsertRoot"));
  assert(insertRoot->next == NULL);

  delete root;

  return manifestInfo;
}
