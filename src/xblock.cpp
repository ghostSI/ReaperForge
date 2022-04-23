#include "xblock.h"

#include "xml.h"

static bool str_ends_with(const char* str, const char* suffix) {
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);

  return (str_len >= suffix_len) && (!memcmp(str + str_len - suffix_len, suffix, suffix_len));
}

static InstrumentFlags instrumentFromName(const char* name)
{
  if (str_ends_with(name, "_Lead"))
    return InstrumentFlags::LeadGuitar;
  if (str_ends_with(name, "_Rhythm"))
    return InstrumentFlags::RhythmGuitar;
  if (str_ends_with(name, "_Bass"))
    return InstrumentFlags::BassGuitar;
  if (str_ends_with(name, "_Vocals"))
    return InstrumentFlags::Vocals;

  assert(false);

  return InstrumentFlags::LeadGuitar;
}

static void readProperty(pugi::xml_node& property, XBlock::Info::Entry::Properties& properties)
{
  const char* name = property.attribute("name").as_string();
  if (0 == strcmp(name, "Header"))
  {
    properties.header = property.child("set").attribute("value").as_string();
    return;
  }
  if (0 == strcmp(name, "Manifest"))
  {
    properties.manifest = property.child("set").attribute("value").as_string();
    return;
  }
  if (0 == strcmp(name, "SngAsset"))
  {
    properties.sngAsset = property.child("set").attribute("value").as_string();
    return;
  }
  if (0 == strcmp(name, "AlbumArtSmall"))
  {
    properties.albumArtSmall = property.child("set").attribute("value").as_string();
    return;
  }
  if (0 == strcmp(name, "AlbumArtMedium"))
  {
    properties.albumArtMedium = property.child("set").attribute("value").as_string();
    return;
  }
  if (0 == strcmp(name, "AlbumArtLarge"))
  {
    properties.albumArtLarge = property.child("set").attribute("value").as_string();
    return;
  }
  if (0 == strcmp(name, "LyricArt"))
  {
    properties.lyricArt = property.child("set").attribute("value").as_string();
    return;
  }
  if (0 == strcmp(name, "ShowLightsXMLAsset"))
  {
    properties.showLightsXMLAsset = property.child("set").attribute("value").as_string();
    return;
  }
  if (0 == strcmp(name, "SoundBank"))
  {
    properties.soundBank = property.child("set").attribute("value").as_string();
    return;
  }
  if (0 == strcmp(name, "PreviewSoundBank"))
  {
    properties.previewSoundBank = property.child("set").attribute("value").as_string();
    return;
  }

  assert(false);
}

XBlock::Info XBlock::readXBlock(const std::vector<u8>& xBlockData)
{
  XBlock::Info xblockInfo;

  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load(reinterpret_cast<const char*>(xBlockData.data()));
  assert(result.status == pugi::status_ok);

  pugi::xml_node entitySet = doc.child("game").child("entitySet");

  for (pugi::xml_node entity : entitySet.children("entity")) {

    XBlock::Info::Entry entry;

    const char* id = entity.attribute("id").as_string();
    memcpy(entry.id, id, 32);

    const char* name = entity.attribute("name").as_string();
    entry.instrument = instrumentFromName(name);

    pugi::xml_node properties = entity.child("properties");
    for (pugi::xml_node property : properties.children("property")) {
      readProperty(property, entry.properties);
    }

    xblockInfo.entries.push_back(entry);
  }

  return xblockInfo;
}