#include "arrangement.h"

#ifdef ARRANGEMENT_XML

#include "xml.h"

Arrangement::Info Arrangement::readArrangement(const std::vector<u8>& arrangementData)
{
  Arrangement::Info arrangementInfo;

  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load(reinterpret_cast<const char*>(arrangementData.data()));
  assert(result.status == pugi::status_ok);

  pugi::xml_node root = doc.child("song");

  arrangementInfo.title = root.child("title").text().as_string();

  pugi::xml_node tuning = root.child("tuning");
  arrangementInfo.tuning.string[0] = tuning.attribute("string0").as_int();
  arrangementInfo.tuning.string[1] = tuning.attribute("string1").as_int();
  arrangementInfo.tuning.string[2] = tuning.attribute("string2").as_int();
  arrangementInfo.tuning.string[3] = tuning.attribute("string3").as_int();
  arrangementInfo.tuning.string[4] = tuning.attribute("string4").as_int();
  arrangementInfo.tuning.string[5] = tuning.attribute("string5").as_int();

  arrangementInfo.capo = root.child("capo").text().as_bool();
  arrangementInfo.artist = root.child("artistName").text().as_string();
  arrangementInfo.albumName = root.child("albumName").text().as_string();
  arrangementInfo.albumYear = root.child("albumYear").text().as_int();
  arrangementInfo.songLength = root.child("songLength").text().as_int();

  pugi::xml_node arrangementProperties = root.child("arrangementProperties");
  arrangementInfo.arrangementProperties.represent = arrangementProperties.attribute("represent").as_bool();
  arrangementInfo.arrangementProperties.standardTuning = arrangementProperties.attribute("standardTuning").as_bool();
  arrangementInfo.arrangementProperties.nonStandardChords = arrangementProperties.attribute("nonStandardChords").as_bool();
  arrangementInfo.arrangementProperties.barreChords = arrangementProperties.attribute("barreChords").as_bool();
  arrangementInfo.arrangementProperties.powerChords = arrangementProperties.attribute("powerChords").as_bool();
  arrangementInfo.arrangementProperties.dropDPower = arrangementProperties.attribute("dropDPower").as_bool();
  arrangementInfo.arrangementProperties.openChords = arrangementProperties.attribute("openChords").as_bool();
  arrangementInfo.arrangementProperties.fingerPicking = arrangementProperties.attribute("fingerPicking").as_bool();
  arrangementInfo.arrangementProperties.pickDirection = arrangementProperties.attribute("pickDirection").as_bool();
  arrangementInfo.arrangementProperties.doubleStops = arrangementProperties.attribute("doubleStops").as_bool();
  arrangementInfo.arrangementProperties.palmMutes = arrangementProperties.attribute("palmMutes").as_bool();
  arrangementInfo.arrangementProperties.harmonics = arrangementProperties.attribute("harmonics").as_bool();
  arrangementInfo.arrangementProperties.pinchHarmonics = arrangementProperties.attribute("pinchHarmonics").as_bool();
  arrangementInfo.arrangementProperties.hopo = arrangementProperties.attribute("hopo").as_bool();
  arrangementInfo.arrangementProperties.tremolo = arrangementProperties.attribute("tremolo").as_bool();
  arrangementInfo.arrangementProperties.slides = arrangementProperties.attribute("slides").as_bool();
  arrangementInfo.arrangementProperties.unpitchedSlides = arrangementProperties.attribute("unpitchedSlides").as_bool();
  arrangementInfo.arrangementProperties.bends = arrangementProperties.attribute("bends").as_bool();
  arrangementInfo.arrangementProperties.tapping = arrangementProperties.attribute("tapping").as_bool();
  arrangementInfo.arrangementProperties.vibrato = arrangementProperties.attribute("vibrato").as_bool();
  arrangementInfo.arrangementProperties.fretHandMutes = arrangementProperties.attribute("fretHandMutes").as_bool();
  arrangementInfo.arrangementProperties.slapPop = arrangementProperties.attribute("slapPop").as_bool();
  arrangementInfo.arrangementProperties.twoFingerPicking = arrangementProperties.attribute("twoFingerPicking").as_bool();
  arrangementInfo.arrangementProperties.fifthsAndOctaves = arrangementProperties.attribute("fifthsAndOctaves").as_bool();
  arrangementInfo.arrangementProperties.syncopation = arrangementProperties.attribute("syncopation").as_bool();
  arrangementInfo.arrangementProperties.bassPick = arrangementProperties.attribute("bassPick").as_bool();
  arrangementInfo.arrangementProperties.sustain = arrangementProperties.attribute("sustain").as_bool();
  arrangementInfo.arrangementProperties.bonusArr = arrangementProperties.attribute("bonusArr").as_bool();
  arrangementInfo.arrangementProperties.Metronome = arrangementProperties.attribute("Metronome").as_bool();
  arrangementInfo.arrangementProperties.pathLead = arrangementProperties.attribute("pathLead").as_bool();
  arrangementInfo.arrangementProperties.pathRhythm = arrangementProperties.attribute("pathRhythm").as_bool();
  arrangementInfo.arrangementProperties.pathBass = arrangementProperties.attribute("pathBass").as_bool();
  arrangementInfo.arrangementProperties.routeMask = arrangementProperties.attribute("routeMask").as_bool();

  return arrangementInfo;
}

#endif // ARRANGEMENT_XML