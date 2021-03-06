
#include "settings.h"
#include "global.h"
#include "file.h"
#include "getopt.h"
#include "installer.h"
#include "helper.h"

#include <SDL2/SDL_video.h>

#include <filesystem>
#include <map>
#include <string>


static std::filesystem::path settingsIniPath("settings.ini");

static std::map<std::string, std::map<std::string, std::string>> commandLineSettings;
static std::map<std::string, std::map<std::string, std::string>> defaultSettings;

static void printUsage() {
  puts("Usage: ReaperForge.exe [OPTION]...\n"
    "Launches the game. Options will overwrite the in-game settings.\n"
    "\n"
    "Options:\n"
    "  -s            Path to settings.ini\n"
    "  -w            Window width\n"
    "  -h            Window height\n"
    "  -f            Fullscreen\n"
    "  -p            Path to song library directory that contains the .psarc files\n"
    "  -l            Show lyrics on highway\n"
    "  -0            Guitar string 0 color (e)\n"
    "  -1            Guitar string 1 color (b)\n"
    "  -2            Guitar string 2 color (G)\n"
    "  -3            Guitar string 3 color (D)\n"
    "  -4            Guitar string 4 color (A)\n"
    "  -5            Guitar string 5 color (E)\n"
    "  -6            Guitar string 6 color (B)\n"
    "  -g            Bass string 0 color (G)\n"
    "  -d            Bass string 1 color (D)\n"
    "  -a            Bass string 2 color (A)\n"
    "  -e            Bass string 3 color (E)\n"
    "  -b            Bass string 4 color (B)\n");

  fflush(stdout);
}

static bool parseCommandLineArgs(int argc, char* argv[]) {
  int c;
  while ((c = getopt(argc, argv, "s:f:w:h:p:l:0:1:2:3:4:5:6:g:d:a:e:b:")) != -1) {
    switch (c) {
    case 's':
      settingsIniPath = optarg;
      break;
    case 'f':
      commandLineSettings["Graphics"]["Fullscreen"] = optarg;
      break;
    case 'w':
      commandLineSettings["Graphics"]["ResolutionWidth"] = optarg;
      break;
    case 'h':
      commandLineSettings["Graphics"]["ResolutionHeight"] = optarg;
      break;
    case 'p':
      commandLineSettings["Library"]["Path"] = optarg;
      break;
    case 'l':
      commandLineSettings["Highway"]["Lyrics"] = optarg;
      break;
    case '0':
      commandLineSettings["Instrument"]["GuitarStringColor0"] = optarg;
      break;
    case '1':
      commandLineSettings["Instrument"]["GuitarStringColor1"] = optarg;
      break;
    case '2':
      commandLineSettings["Instrument"]["GuitarStringColor2"] = optarg;
      break;
    case '3':
      commandLineSettings["Instrument"]["GuitarStringColor3"] = optarg;
      break;
    case '4':
      commandLineSettings["Instrument"]["GuitarStringColor4"] = optarg;
      break;
    case '5':
      commandLineSettings["Instrument"]["GuitarStringColor5"] = optarg;
      break;
    case '6':
      commandLineSettings["Instrument"]["GuitarStringColor6"] = optarg;
      break;
    case 'g':
      commandLineSettings["Instrument"]["BassStringColor6"] = optarg;
      break;
    case 'd':
      commandLineSettings["Instrument"]["BassStringColor6"] = optarg;
      break;
    case 'a':
      commandLineSettings["Instrument"]["BassStringColor6"] = optarg;
      break;
    case 'e':
      commandLineSettings["Instrument"]["BassStringColor6"] = optarg;
      break;
    case 'b':
      commandLineSettings["Instrument"]["BassStringColor6"] = optarg;
      break;
    default:
      printUsage();
      return false;
    }
  }

  return true;
}

//static const char* themes[] = {
//        "#FDB0C0#4A0100#FD4659", "#9DBCD4#FFFD01#6D5ACF", "#048243#32BF84#CAFFFB", "#FFAB0F#247AFD#FE46A5",
//        "#ED0DD9#C1C6FC#BE03FD", "#00022E#FF073A#02066F", "#475F94#FDDC5C#FA4224", "#343837#03719C#0F9B8E",
//        "#FFBACD#DA467D#CB0162", "#C6FCFF#CEA2FD#9900FA", "#D8DCD6#3B719F#8F8CE7", "#A75E09#DBB40C#D5174E",
//        "#107AB0#FDC1C5#FD5956", "#017374#D0FEFE#1F3B4D", "#FFC513#FF724C#730039", "#F19E8E#98756F#58656D",
//        "#47C072#FF0789#FCB005", "#728639#062E03#FF964F", "#4984B8#DFC5FE#C83CB9", "#FE2C54#26F7FD#3E82FC",
//        "#82A67D#2DFE54#825F87", "#FE019A#5A06EF#FCFC81", "#000133#FC86AA#D8DCD6", "#FDB147#3F012C#08787F",
//        "#017B92#FF6BB5#B36FF6", "#070D0D#B30049#EDC8FE", "#FEA993#FE02A2#FDEE73", "#5D21D0#10A674#BFF128",
//        "#FFD8B1#C27F79#610023", "#FDC1C5#F6688E#805B87", "#FCC006#FDAB48#FF7052", "#DFC5FE#5D1451#04D9FF",
//        "#000000#FB5681#95A3A6", "#E17701#C65102#FFD8B1", "#6241C7#89A0B0#FFC5CB", "#214761#CB7723#BB3F3F",
//        "#D1768F#610023#F8481C", "#FCF679#41FDFE#FF81C0", "#42B395#044A05#BDF8A3", "#FC824A#B00149#0C1793",
//        "#FFD1DF#922B05#FEA993", "#6C3461#94568C#FDB915", "#017371#56FCA2#0E87CC", "#E78EA5#FEB8CE#373737",
//        "#677A04#FFE5AD#33B864", "#B04E0F#FA5FF7#FFC512", "#FF964F#D46A7E#59656D", "#76FF7B#FF69AF#5729CE",
//        "#1B2431#016795#1E488F", "#7BC8F6#A7FFB5#FEB2D0", "#FF9A8A#2E5A88#A09BD7", "#D3494E#FFE5AD#13BBAF",
//        "#FFFF14#CB0162#062E03", "#FDDC5C#FDFDFE#BDF6FE", "#FCB001#6B7C85#040348", "#005249#FF0490#FFBACD",
//        "#FF6163#137E6D#CFFDBC", "#AFA88A#FD5956#CD5908", "#FFFF7E#8AB8FE#665FD1", "#7E4071#FF0789#2A0134",
//        "#F5054F#3F012C#EFC0FE", "#FFA62B#1B2431#75B3E7", "#4E0550#CA6641#005249", "#FD8D49#FDDE6C#F43605",
//        "#000133#A442A0#EFC0FE", "#26F7FD#3D7AFD#1F0954", "#FEDF08#B7C9E2#5684AE", "#B79400#FD3C06#985E2B",
//        "#1FA774#90E4C1#FFD1DF", "#36013F#8F8CE7#017A79", "#B790D4#7AF9AB#0A481E", "#FCE166#069AF3#34013F",
//        "#31668A#F10C45#6B7C85", "#6C3461#D9544D#048243", "#F5BF03#B790D4#3C4142", "#C3FBF4#5684AE#000000",
//        "#F19E8E#F6688E#F8D561", "#FC824A#C65102#3C0008", "#4B006E#8AB8FE#000133", "#0CB577#014D4E#002D04",
//        "#3C4142#13BBAF#EDC8FF", "#FDC1C5#FFDA03#85A3B2", "#9DFF00#FE86A4#FE019A", "#FF9408#017374#014D4E",
//        "#826D8C#C88D94#84597E", "#FDE166#D46A7E#015F6B", "#B9CC81#FEFFCA#748500", "#F6688E#CB416B#FFB19A",
//        "#D6B4FC#5D21D0#01F9C6", "#FFFFE4#B17261#8F1402", "#FAFE4B#FED0FC#FB5581", "#FFC512#CFAF7B#0485D1",
//        "#014D4E#F4320C#3F012C", "#6C3461#FFFE71#C3909B", "#BFFE27#FD4B04#D726DE", "#6A6E09#FFE5AD#5170D7",
//        "#F10C45#20C073#7D7F7C", "#B9FF66#C071FE#5B7C99", "#341C02#C9B003#FFFD78", "#0504AA#FF63E9#017374",
//        "#5A86AD#FDAA47#C85A53", "#7D7F7B#1F6357#968A84", "#E6DAA6#CF0134#FD3C07", "#FEAD01#2479FD#CC6743"
//};

static std::map<std::string, std::map<std::string, std::string>> serialize(const Settings::Info& settings)
{
  std::map<std::string, std::map<std::string, std::string>> serializedSettings =
  {
    {
      "Audio",
      {
        { "BufferSize",         std::to_string(settings.audioBufferSize) },
        { "ChannelInstrument0", std::to_string(settings.audioChannelInstrument[0]) },
        { "ChannelInstrument1", std::to_string(settings.audioChannelInstrument[1]) },
        { "SampleRate",         std::to_string(settings.audioSampleRate) },
        { "SignalChain",        std::to_string(to_underlying(settings.audioSignalChain)) }
      }
    },
    {
      "Camera",
      {
        { "BreakRadius",       std::to_string(settings.cameraBreakRadius) },
        { "FieldOfView",       std::to_string(settings.cameraFieldOfView) },
        { "MaximumBreakForce", std::to_string(settings.cameraMaximumBreakForce) },
        { "MaximumForce",      std::to_string(settings.cameraMaximumForce) },
        { "MaximumVelocity",   std::to_string(settings.cameraMaximumVelocity) },
        { "XFactor",           std::to_string(settings.cameraXFactor) },
        { "XOffset",           std::to_string(settings.cameraXOffset) },
        { "XRotation",         std::to_string(settings.cameraXRotation) },
        { "YFactor",           std::to_string(settings.cameraYFactor) },
        { "YOffset",           std::to_string(settings.cameraYOffset) },
        { "YRotation",         std::to_string(settings.cameraYRotation) },
        { "ZFactor",           std::to_string(settings.cameraZFactor) },
        { "ZOffset",           std::to_string(settings.cameraZOffset) }
      }
    },
    {
      "Graphics",
      {
        { "Fullscreen",       std::to_string(to_underlying(settings.graphicsFullscreen)) },
        { "WindowWidth",  std::to_string(settings.graphicsWindowWidth) },
        { "WindowHeight", std::to_string(settings.graphicsWindowHeight) },
      }
    },
    {
      "Highway",
      {
        { "AnchorColor0",            hexColor(settings.highwayAnchorColor[0]) },
        { "AnchorColor1",            hexColor(settings.highwayAnchorColor[1]) },
        { "BackgroundColor",         hexColor(settings.highwayBackgroundColor) },
        { "ChordBoxColor0",          hexColor(settings.highwayChordBoxColor[0]) },
        { "ChordBoxColor1",          hexColor(settings.highwayChordBoxColor[1]) },
        { "ChordNameColor",          hexColor(settings.highwayChordNameColor) },
        { "DetectorColor",           hexColor(settings.highwayDetectorColor) },
        { "DotInlayColor0",          hexColor(settings.highwayDotInlayColor[0]) },
        { "DotInlayColor1",          hexColor(settings.highwayDotInlayColor[1]) },
        { "Ebeat",                   std::to_string(settings.highwayEbeat) },
        { "EbeatColor0",             hexColor(settings.highwayEbeatColor[0]) },
        { "EbeatColor1",             hexColor(settings.highwayEbeatColor[1]) },
        { "FingerNumberColor",       hexColor(settings.highwayFingerNumberColor) },
        { "FretNoteNames",           std::to_string(settings.highwayFretNoteNames) },
        { "FretNumberColor0",        hexColor(settings.highwayFretNumberColor[0])},
        { "FretNumberColor1",        hexColor(settings.highwayFretNumberColor[1])},
        { "FretNumberColor2",        hexColor(settings.highwayFretNumberColor[2])},
        { "FretboardNoteNameColor0", hexColor(settings.highwayFretboardNoteNameColor[0]) },
        { "FretboardNoteNameColor1", hexColor(settings.highwayFretboardNoteNameColor[1]) },
        { "GroundFretColor0",        hexColor(settings.highwayGroundFretColor[0]) },
        { "GroundFretColor1",        hexColor(settings.highwayGroundFretColor[1]) },
        { "Lyrics",                  std::to_string(settings.highwayLyrics) },
        { "LyricsColor0",            hexColor(settings.highwayLyricsColor[0]) },
        { "LyricsColor1",            hexColor(settings.highwayLyricsColor[1]) },
        { "LyricsColor2",            hexColor(settings.highwayLyricsColor[2]) },
        { "PhraseColor0",            hexColor(settings.highwayPhraseColor[0]) },
        { "PhraseColor1",            hexColor(settings.highwayPhraseColor[1]) },
        { "PhraseColor2",            hexColor(settings.highwayPhraseColor[2]) },
        { "PhraseColor3",            hexColor(settings.highwayPhraseColor[3]) },
        { "ReverseStrings",          std::to_string(settings.highwayReverseStrings) },
        { "SongInfo",                std::to_string(settings.highwaySongInfo) },
        { "SongInfoColor",           hexColor(settings.highwaySongInfoColor) },
        { "SpeedMultiplier",         std::to_string(settings.highwaySpeedMultiplier) },
        { "StringNoteNames",         std::to_string(settings.highwayStringNoteNames) },
        { "ToneAssignment",          std::to_string(settings.highwayToneAssignment) },
        { "ToneAssignmentColor",     hexColor(settings.highwayToneAssignmentColor) }
      }
    },
    {
      "Instrument",
      {
        { "Bass5StringTuning0",     std::to_string(settings.instrumentBass5StringTuning[0]) },
        { "Bass5StringTuning1",     std::to_string(settings.instrumentBass5StringTuning[1]) },
        { "Bass5StringTuning2",     std::to_string(settings.instrumentBass5StringTuning[2]) },
        { "Bass5StringTuning3",     std::to_string(settings.instrumentBass5StringTuning[3]) },
        { "BassFirstWoundString",   std::to_string(settings.instrumentBassFirstWoundString) },
        { "BassStringColor0",       hexColor(settings.instrumentBassStringColor[0]) },
        { "BassStringColor1",       hexColor(settings.instrumentBassStringColor[1]) },
        { "BassStringColor2",       hexColor(settings.instrumentBassStringColor[2]) },
        { "BassStringColor3",       hexColor(settings.instrumentBassStringColor[3]) },
        { "BassStringColor4",       hexColor(settings.instrumentBassStringColor[4]) },
        { "Guitar7StringTuning0",   std::to_string(settings.instrumentGuitar7StringTuning[0]) },
        { "Guitar7StringTuning1",   std::to_string(settings.instrumentGuitar7StringTuning[1]) },
        { "Guitar7StringTuning2",   std::to_string(settings.instrumentGuitar7StringTuning[2]) },
        { "Guitar7StringTuning3",   std::to_string(settings.instrumentGuitar7StringTuning[3]) },
        { "Guitar7StringTuning4",   std::to_string(settings.instrumentGuitar7StringTuning[4]) },
        { "Guitar7StringTuning5",   std::to_string(settings.instrumentGuitar7StringTuning[5]) },
        { "GuitarFirstWoundString", std::to_string(settings.instrumentGuitarFirstWoundString) },
        { "GuitarStringColor0",     hexColor(settings.instrumentGuitarStringColor[0]) },
        { "GuitarStringColor1",     hexColor(settings.instrumentGuitarStringColor[1]) },
        { "GuitarStringColor2",     hexColor(settings.instrumentGuitarStringColor[2]) },
        { "GuitarStringColor3",     hexColor(settings.instrumentGuitarStringColor[3]) },
        { "GuitarStringColor4",     hexColor(settings.instrumentGuitarStringColor[4]) },
        { "GuitarStringColor5",     hexColor(settings.instrumentGuitarStringColor[5]) },
        { "GuitarStringColor6",     hexColor(settings.instrumentGuitarStringColor[6]) }
      }
    },
    {
      "Midi",
      {
        { "AutoConnectDevices", settings.autoConnectDevices },
      }
    },
    {
      "Paths",
      {
#ifdef SUPPORT_BNK
        { "Bnk", settings.bnkPath },
#endif // SUPPORT_BNK
        { "Psarc", settings.psarcPath },
        { "Vst", settings.vstPath },
        { "Vst3", settings.vst3Path }
      }
    },
    {
      "Mixer",
      {
        { "MusicVolume",      std::to_string(settings.mixerMusicVolume) },
        { "Guitar1Volume",    std::to_string(settings.mixerGuitar1Volume) },
        { "Bass1Volume",      std::to_string(settings.mixerBass1Volume) },
        { "Guitar2Volume",    std::to_string(settings.mixerGuitar2Volume) },
        { "Bass2Volume",      std::to_string(settings.mixerBass2Volume) },
        { "MicrophoneVolume", std::to_string(settings.mixerMicrophoneVolume) }
      }
    },
    {
      "Profile",
      {
        { "PreferedSongFormat", std::to_string(to_underlying(settings.profilePreferedSongFormat)) },
        { "SaveMode", std::to_string(to_underlying(settings.profileSaveMode)) }
      }
    },
    {
      "Ui",
      {
        { "Scale", std::to_string(settings.uiScale) }
      }
    }
  };

#ifdef SUPPORT_MIDI
  for (i32 i = 0; i < NUM(Const::midiBindingsNames); ++i)
  {
    if (Global::settings.midiBinding[i] == 0xFF)
      serializedSettings["Midi"].insert({ std::string("Binding") + Const::midiBindingsNames[i], "" });
    else
      serializedSettings["Midi"].insert({ std::string("Binding") + Const::midiBindingsNames[i], std::to_string(Global::settings.midiBinding[i]) });
  }
#endif // SUPPORT_MIDI

  return serializedSettings;
}

static Settings::Info deserialize(const std::map<std::string, std::map<std::string, std::string>>& serializedSettings)
{
  Settings::Info settings =
  {
    .audioBufferSize = atoi(serializedSettings.at("Audio").at("BufferSize").c_str()),
    .audioChannelInstrument = {
      atoi(serializedSettings.at("Audio").at("ChannelInstrument0").c_str()),
      atoi(serializedSettings.at("Audio").at("ChannelInstrument1").c_str())
    },
    .audioSampleRate = atoi(serializedSettings.at("Audio").at("SampleRate").c_str()),
    .audioSignalChain = SignalChain(atoi(serializedSettings.at("Audio").at("SignalChain").c_str())),
    .cameraBreakRadius = f32(atof(serializedSettings.at("Camera").at("BreakRadius").c_str())),
    .cameraFieldOfView = f32(atof(serializedSettings.at("Camera").at("FieldOfView").c_str())),
    .cameraMaximumBreakForce = f32(atof(serializedSettings.at("Camera").at("MaximumBreakForce").c_str())),
    .cameraMaximumForce = f32(atof(serializedSettings.at("Camera").at("MaximumForce").c_str())),
    .cameraMaximumVelocity = f32(atof(serializedSettings.at("Camera").at("MaximumVelocity").c_str())),
    .cameraXFactor = f32(atof(serializedSettings.at("Camera").at("XFactor").c_str())),
    .cameraXOffset = f32(atof(serializedSettings.at("Camera").at("XOffset").c_str())),
    .cameraXRotation = f32(atof(serializedSettings.at("Camera").at("XRotation").c_str())),
    .cameraYFactor = f32(atof(serializedSettings.at("Camera").at("YFactor").c_str())),
    .cameraYOffset = f32(atof(serializedSettings.at("Camera").at("YOffset").c_str())),
    .cameraYRotation = f32(atof(serializedSettings.at("Camera").at("YRotation").c_str())),
    .cameraZFactor = f32(atof(serializedSettings.at("Camera").at("ZFactor").c_str())),
    .cameraZOffset = f32(atof(serializedSettings.at("Camera").at("ZOffset").c_str())),
    .graphicsFullscreen = FullscreenMode(atoi(serializedSettings.at("Graphics").at("Fullscreen").c_str())),
    .graphicsWindowWidth = atoi(serializedSettings.at("Graphics").at("WindowWidth").c_str()),
    .graphicsWindowHeight = atoi(serializedSettings.at("Graphics").at("WindowHeight").c_str()),
    .highwayAnchorColor = {
      colorVec4(serializedSettings.at("Highway").at("AnchorColor0")),
      colorVec4(serializedSettings.at("Highway").at("AnchorColor1"))
    },
    .highwayBackgroundColor = colorVec4(serializedSettings.at("Highway").at("BackgroundColor")),
    .highwayChordBoxColor = {
      colorVec4(serializedSettings.at("Highway").at("ChordBoxColor0")),
      colorVec4(serializedSettings.at("Highway").at("ChordBoxColor1"))
    },
    .highwayChordNameColor = colorVec4(serializedSettings.at("Highway").at("ChordNameColor")),
    .highwayDetectorColor = colorVec4(serializedSettings.at("Highway").at("DetectorColor")),
    .highwayDotInlayColor = {
      colorVec4(serializedSettings.at("Highway").at("DotInlayColor0")),
      colorVec4(serializedSettings.at("Highway").at("DotInlayColor1"))
    },
    .highwayEbeat = bool(atoi(serializedSettings.at("Highway").at("Ebeat").c_str())),
    .highwayEbeatColor = {
      colorVec4(serializedSettings.at("Highway").at("EbeatColor0")),
      colorVec4(serializedSettings.at("Highway").at("EbeatColor1"))
    },
    .highwayFingerNumberColor = colorVec4(serializedSettings.at("Highway").at("FingerNumberColor")),
    .highwayFretNoteNames = bool(atoi(serializedSettings.at("Highway").at("FretNoteNames").c_str())),
    .highwayFretNumberColor = {
      colorVec4(serializedSettings.at("Highway").at("FretNumberColor0")),
      colorVec4(serializedSettings.at("Highway").at("FretNumberColor1")),
      colorVec4(serializedSettings.at("Highway").at("FretNumberColor2"))
    },
    .highwayFretboardNoteNameColor = {
      colorVec4(serializedSettings.at("Highway").at("FretboardNoteNameColor0")),
      colorVec4(serializedSettings.at("Highway").at("FretboardNoteNameColor1"))
    },
    .highwayGroundFretColor = {
      colorVec4(serializedSettings.at("Highway").at("GroundFretColor0")),
      colorVec4(serializedSettings.at("Highway").at("GroundFretColor1"))
    },
    .highwayLyrics = bool(atoi(serializedSettings.at("Highway").at("Lyrics").c_str())),
    .highwayLyricsColor = {
      colorVec4(serializedSettings.at("Highway").at("LyricsColor0")),
      colorVec4(serializedSettings.at("Highway").at("LyricsColor1")),
      colorVec4(serializedSettings.at("Highway").at("LyricsColor2"))
    },
    .highwayPhraseColor = {
      colorVec4(serializedSettings.at("Highway").at("PhraseColor0")),
      colorVec4(serializedSettings.at("Highway").at("PhraseColor1")),
      colorVec4(serializedSettings.at("Highway").at("PhraseColor2")),
      colorVec4(serializedSettings.at("Highway").at("PhraseColor3"))
    },
    .highwayReverseStrings = bool(atoi(serializedSettings.at("Highway").at("ReverseStrings").c_str())),
    .highwaySongInfo = bool(atoi(serializedSettings.at("Highway").at("SongInfo").c_str())),
    .highwaySongInfoColor = colorVec4(serializedSettings.at("Highway").at("SongInfoColor")),
    .highwaySpeedMultiplier = f32(atof(serializedSettings.at("Highway").at("SpeedMultiplier").c_str())),
    .highwayStringNoteNames = bool(atoi(serializedSettings.at("Highway").at("StringNoteNames").c_str())),
    .highwayToneAssignment = bool(atoi(serializedSettings.at("Highway").at("ToneAssignment").c_str())),
    .highwayToneAssignmentColor = colorVec4(serializedSettings.at("Highway").at("ToneAssignmentColor")),
    .instrumentBass5StringTuning = {
      atoi(serializedSettings.at("Instrument").at("Bass5StringTuning0").c_str()),
      atoi(serializedSettings.at("Instrument").at("Bass5StringTuning1").c_str()),
      atoi(serializedSettings.at("Instrument").at("Bass5StringTuning2").c_str()),
      atoi(serializedSettings.at("Instrument").at("Bass5StringTuning3").c_str()),
    },
    .instrumentBassFirstWoundString = atoi(serializedSettings.at("Instrument").at("BassFirstWoundString").c_str()),
    .instrumentBassStringColor = {
      colorVec4(serializedSettings.at("Instrument").at("BassStringColor0")),
      colorVec4(serializedSettings.at("Instrument").at("BassStringColor1")),
      colorVec4(serializedSettings.at("Instrument").at("BassStringColor2")),
      colorVec4(serializedSettings.at("Instrument").at("BassStringColor3")),
      colorVec4(serializedSettings.at("Instrument").at("BassStringColor4"))
    },
    .instrumentGuitar7StringTuning = {
      atoi(serializedSettings.at("Instrument").at("Guitar7StringTuning0").c_str()),
      atoi(serializedSettings.at("Instrument").at("Guitar7StringTuning1").c_str()),
      atoi(serializedSettings.at("Instrument").at("Guitar7StringTuning2").c_str()),
      atoi(serializedSettings.at("Instrument").at("Guitar7StringTuning3").c_str()),
      atoi(serializedSettings.at("Instrument").at("Guitar7StringTuning4").c_str()),
      atoi(serializedSettings.at("Instrument").at("Guitar7StringTuning5").c_str()),
    },
    .instrumentGuitarFirstWoundString = atoi(serializedSettings.at("Instrument").at("GuitarFirstWoundString").c_str()),
    .instrumentGuitarStringColor = {
      colorVec4(serializedSettings.at("Instrument").at("GuitarStringColor0")),
      colorVec4(serializedSettings.at("Instrument").at("GuitarStringColor1")),
      colorVec4(serializedSettings.at("Instrument").at("GuitarStringColor2")),
      colorVec4(serializedSettings.at("Instrument").at("GuitarStringColor3")),
      colorVec4(serializedSettings.at("Instrument").at("GuitarStringColor4")),
      colorVec4(serializedSettings.at("Instrument").at("GuitarStringColor5")),
      colorVec4(serializedSettings.at("Instrument").at("GuitarStringColor6"))
    },
#ifdef SUPPORT_MIDI
    .autoConnectDevices = serializedSettings.at("Midi").at("AutoConnectDevices"),
#endif // SUPPORT_MIDI
#ifdef SUPPORT_BNK
    .bnkPath = serializedSettings.at("Paths").at("Bnk"),
#endif SUPPORT_BNK
    .psarcPath = serializedSettings.at("Paths").at("Psarc"),
    .vstPath = serializedSettings.at("Paths").at("Vst"),
    .vst3Path = serializedSettings.at("Paths").at("Vst3"),
    .mixerMusicVolume = atoi(serializedSettings.at("Mixer").at("MusicVolume").c_str()),
    .mixerGuitar1Volume = atoi(serializedSettings.at("Mixer").at("Guitar1Volume").c_str()),
    .mixerBass1Volume = atoi(serializedSettings.at("Mixer").at("Bass1Volume").c_str()),
    .profilePreferedSongFormat = SongFormat(atoi(serializedSettings.at("Profile").at("PreferedSongFormat").c_str())),
    .profileSaveMode = SaveMode(atoi(serializedSettings.at("Profile").at("SaveMode").c_str())),
    .uiScale = f32(atof(serializedSettings.at("Ui").at("Scale").c_str()))
  };

#ifdef SUPPORT_MIDI
  for (i32 i = 0; i < NUM(Const::midiBindingsNames); ++i)
  {
    if (!serializedSettings.at("Midi").at(std::string("Binding") + Const::midiBindingsNames[i]).empty())
      settings.midiBinding[i] = atoi(serializedSettings.at("Midi").at(std::string("Binding") + Const::midiBindingsNames[i]).c_str());
  }
#endif // SUPPORT_MIDI

  return settings;
}

bool Settings::init(int argc, char* argv[]) {

  if (!parseCommandLineArgs(argc, argv))
    return false;

  defaultSettings = serialize(Global::settings);

  if (Global::isInstalled)
    Global::settings = deserialize(File::loadIni("settings.ini"));

  return true;
}

void Settings::fini() {
  if (Global::isInstalled)
  {
    const auto serializedSettings = serialize(Global::settings);
    File::saveIni(settingsIniPath.string().c_str(), serializedSettings);
  }
}

//static std::vector<Settings::Resolution> getSupportedDisplayResolutions() {
//    std::vector<Settings::Resolution> supportedDisplayResolutions;
//
//    const i32 displayCount = SDL_GetNumVideoDisplays();
//    for (i32 displayIdx = 0; displayIdx <= 1; ++displayIdx) {
//        const i32 modeCount = SDL_GetNumDisplayModes(displayIdx);
//        for (i32 modeIdx = 0; modeIdx <= modeCount; ++modeIdx) {
//            SDL_DisplayMode mode = {SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0};
//
//            if (SDL_GetDisplayMode(displayIdx, modeIdx, &mode) == 0) {
//                const size_t size = supportedDisplayResolutions.size();
//                if (size >= 1 && (supportedDisplayResolutions.at(size - 1).w == mode.w &&
//                                  supportedDisplayResolutions.at(size - 1).h))
//                    continue;
//
//                const Settings::Resolution resolution{
//                        .w = mode.w,
//                        .h = mode.h,
//                };
//                supportedDisplayResolutions.push_back(resolution);
//            }
//        }
//    }
//
//    return supportedDisplayResolutions;
//}
