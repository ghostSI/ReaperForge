#include "xblock.h"

#include "xml.h"

#include <string.h>

static bool str_ends_with(const char* str, const char* suffix) {
  const u64 str_len = strlen(str);
  const u64 suffix_len = strlen(suffix);

  return (str_len >= suffix_len) && (!memcmp(str + str_len - suffix_len, suffix, suffix_len));
}

static InstrumentFlags instrumentFlagsFromName(const char* name)
{
  if (str_ends_with(name, "_Lead"))
    return InstrumentFlags::LeadGuitar;
  if (str_ends_with(name, "_Rhythm"))
    return InstrumentFlags::RhythmGuitar;
  if (str_ends_with(name, "_Bass"))
    return InstrumentFlags::BassGuitar;
  if (str_ends_with(name, "_Lead2"))
    return InstrumentFlags::LeadGuitar | InstrumentFlags::Second;
  if (str_ends_with(name, "_Rhythm2"))
    return InstrumentFlags::RhythmGuitar | InstrumentFlags::Second;
  if (str_ends_with(name, "_Bass2"))
    return InstrumentFlags::BassGuitar | InstrumentFlags::Second;
  if (str_ends_with(name, "_Lead3"))
    return InstrumentFlags::LeadGuitar | InstrumentFlags::Third;
  if (str_ends_with(name, "_Rhythm3"))
    return InstrumentFlags::RhythmGuitar | InstrumentFlags::Third;
  if (str_ends_with(name, "_Bass3"))
    return InstrumentFlags::BassGuitar | InstrumentFlags::Third;

  // TODO: research the ones below.
  if (str_ends_with(name, "_Combo")) // Lead and Rhythm?
    return InstrumentFlags::LeadGuitar;
  if (str_ends_with(name, "_Combo2"))
    return InstrumentFlags::LeadGuitar | InstrumentFlags::Second;
  if (str_ends_with(name, "_Combo3"))
    return InstrumentFlags::LeadGuitar | InstrumentFlags::Third;
  if (str_ends_with(name, "_Vocals"))
    return InstrumentFlags::none;
  if (str_ends_with(name, "_JVocals"))
    return InstrumentFlags::none;
  if (str_ends_with(name, "_ShowLights"))
    return InstrumentFlags::none;

  assert(false);

  return InstrumentFlags::none;
}

#ifdef XBLOCK_FULL
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
#endif // XBLOCK_FULL

XBlock::Info XBlock::readXBlock(const std::vector<u8>& xBlockData)
{
  XBlock::Info xblockInfo;

  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_string(reinterpret_cast<const char*>(xBlockData.data()));
#ifndef XML_IGNORE_ERROR
  assert(result.status == pugi::status_ok);
#endif // XML_IGNORE_ERROR

  pugi::xml_node entitySet = doc.child("game").child("entitySet");

  for (pugi::xml_node entity : entitySet.children("entity")) {

    XBlock::Info::Entry entry;

    const char* id = entity.attribute("id").as_string();
    assert(strlen(id) == 32);
    memcpy(entry.id, id, 32);

    const char* name = entity.attribute("name").as_string();
    entry.instrumentFlags = instrumentFlagsFromName(name);
    if (entry.instrumentFlags == InstrumentFlags::none)
      continue;

#ifdef XBLOCK_FULL
    pugi::xml_node properties = entity.child("properties");
    for (pugi::xml_node property : properties.children("property")) {
      readProperty(property, entry.properties);
    }
#endif // XBLOCK_FULL

    xblockInfo.entries.push_back(entry);
  }

  return xblockInfo;
}
