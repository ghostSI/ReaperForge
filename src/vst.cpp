#include "vst.h"

#ifdef SUPPORT_VST

#include "global.h"

#include "SDL2/SDL_syswm.h"

#include <windows.h> 

#include <filesystem>

struct AEffect
{
  i32 magic;
  intptr_t(*dispatcher)(AEffect*, i32, i32, intptr_t, void*, f32);
  void (*process)(AEffect*, f32**, f32**, i32);
  void (*setParameter)(AEffect*, i32, f32);
  f32(*getParameter)(AEffect*, i32);
  i32 numPrograms;
  i32 numParams;
  i32 numInputs;
  i32 numOutputs;
  i32 flags;
  void* ptr1;
  void* ptr2;
  i32 initialDelay;
  i32 empty3a;
  i32 empty3b;
  f32 unkown_float;
  void* ptr3;
  void* user;
  int32_t uniqueID;
  int32_t version;
  void (*processReplacing)(AEffect*, f32**, f32**, i32);
};

struct VstPlugin
{
  AEffect* aEffect;
  i32 vstVersion;
  std::string name;
  std::string vendor;
  i32 version;
  bool interactive;
  u32 audioIns;
  u32 audioOuts;
  i32 midiIns;
  i32 midiOuts;
  bool automatable;
};

static std::vector<VstPlugin> vstPlugins;

typedef intptr_t(*audioMasterCallback)(AEffect*, int32_t, int32_t, intptr_t, void*, f32);
typedef AEffect* (*vstPluginMain)(audioMasterCallback audioMaster);

enum AudioMaster : i32
{
  automate = 0,
  version = 1,
  currentId = 2,
  idle = 3,
  pinConnected = 4,
  wantMidi = 6,
  getTime = 7,
  processEvents = 8,
  setTime = 9,
  tempoAt = 10,
  getNumAutomatableParameters = 11,
  getParameterQuantization = 12,
  ioChanged = 13,
  needIdle = 14,
  sizeWindow = 15,
  getSampleRate = 16,
  getBlockSize = 17,
  getInputLatency = 18,
  getOutputLatency = 19,
  getPreviousPlug = 20,
  getNextPlug = 21,
  willReplaceOrAccumulate = 22,
  getCurrentProcessLevel = 23,
  getAutomationState = 24,
  offlineStart = 25,
  offlineRead = 26,
  offlineWrite = 27,
  offlineGetCurrentPass = 28,
  offlineGetCurrentMetaPass = 29,
  setOutputSampleRate = 30,
  getSpeakerArrangement = 31,
  getVendorString = 32,
  getProductString = 33,
  getVendorVersion = 34,
  vendorSpecific = 35,
  setIcon = 36,
  canDo = 37,
  getLanguage = 38,
  openWindow = 39,
  closeWindow = 40,
  getDirectory = 41,
  updateDisplay = 42,
  beginEdit = 43,
  endEdit = 44,
  openFileSelector = 45,
  closeFileSelector = 46,
  editFile = 47,
  getChunkFile = 48,
  getInputSpeakerArrangement = 49,
};

enum EffFlags : i32
{
  HasEditor = 1,
  CanReplacing = 1 << 4,
  ProgramChunks = 1 << 5,
  IsSynth = 1 << 8
};

enum Eff : i32
{
  Open = 0,
  Close = 1,
  SetProgram = 2,
  GetProgram = 3,
  SetProgramName = 4,
  GetProgramName = 5,
  GetParamLabel = 6,
  GetParamDisplay = 7,
  GetParamName = 8,
  SetSampleRate = 10,
  SetBlockSize = 11,
  MainsChanged = 12,
  EditGetRect = 13,
  EditOpen = 14,
  EditClose = 15,
  EditIdle = 19,
  EditTop = 20,
  Identify = 22,
  GetChunk = 23,
  SetChunk = 24,
  ProcessEvents = 25,
  CanBeAutomated = 26,
  GetProgramNameIndexed = 29,
  GetPlugCategory = 35,
  GetEffectName = 45,
  GetParameterProperties = 56,
  GetVendorString = 47,
  GetProductString = 48,
  GetVendorVersion = 49,
  CanDo = 51,
  Idle = 53,
  GetVstVersion = 58,
  BeginSetProgram = 67,
  EndSetProgram = 68,
  ShellGetNextPlugin = 70,
  StartProcess = 71,
  StopProcess = 72,
  BeginLoadBank = 75,
  BeginLoadProgram = 76
};

#define CCONST(a, b, c, d)( ( ( (i32) a ) << 24 ) |      \
            ( ( (i32) b ) << 16 ) |    \
            ( ( (i32) c ) << 8 ) |     \
            ( ( (i32) d ) << 0 ) )

const i32 kEffectMagic = CCONST('V', 's', 't', 'P');
const i32 kVstLangEnglish = 1;
const i32 kVstMidiType = 1;

const i32 kVstNanosValid = 1 << 8;
const i32 kVstPpqPosValid = 1 << 9;
const i32 kVstTempoValid = 1 << 10;
const i32 kVstBarsValid = 1 << 11;
const i32 kVstCyclePosValid = 1 << 12;
const i32 kVstTimeSigValid = 1 << 13;
const i32 kVstSmpteValid = 1 << 14;   // from Ardour
const i32 kVstClockValid = 1 << 15;   // from Ardour

const i32 kVstTransportPlaying = 1 << 1;
const i32 kVstTransportCycleActive = 1 << 2;
const i32 kVstTransportChanged = 1;


#define REAPERFORGE_VERSION 0
#define REAPERFORGE_RELEASE 0
#define REAPERFORGE_REVISION 0
#define REAPERFORGE_MODLEVEL 0

static intptr_t AudioMaster(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, f32 opt)
{
  switch (opcode)
  {
  case AudioMaster::version:
    return (intptr_t)2400;

  case AudioMaster::currentId:
    return 0;

  case AudioMaster::getVendorString:
    strcpy((char*)ptr, "Audacity Team");    // Do not translate, max 64 + 1 for null terminator
    return 1;

  case AudioMaster::getProductString:
    strcpy((char*)ptr, "Audacity");         // Do not translate, max 64 + 1 for null terminator
    return 1;

  case AudioMaster::getVendorVersion:
    return (intptr_t)(REAPERFORGE_VERSION << 24 |
      REAPERFORGE_RELEASE << 16 |
      REAPERFORGE_REVISION << 8 |
      REAPERFORGE_MODLEVEL);

  case AudioMaster::needIdle:
    return 0;

  case AudioMaster::updateDisplay:
    return 0;

  case AudioMaster::getTime:
    return 0;

  case AudioMaster::ioChanged:
    return 0;

  case AudioMaster::getSampleRate:
    return (intptr_t)Global::settings.audioSampleRate;

  case AudioMaster::idle:
    return 1;

  case AudioMaster::getCurrentProcessLevel:
    return 0;

  case AudioMaster::getLanguage:
    return kVstLangEnglish;

  case AudioMaster::willReplaceOrAccumulate:
    return 1;

  case AudioMaster::sizeWindow:
    return 1;

  case AudioMaster::canDo:
  {
    char* s = (char*)ptr;
    if (strcmp(s, "acceptIOChanges") == 0 ||
      strcmp(s, "sendVstTimeInfo") == 0 ||
      strcmp(s, "startStopProcess") == 0 ||
      strcmp(s, "shellCategory") == 0 ||
      strcmp(s, "sizeWindow") == 0)
    {
      return 1;
    }
    return 0;
  }

  case AudioMaster::beginEdit:
  case AudioMaster::endEdit:
    return 0;

  case AudioMaster::automate:
    return 0;

  case AudioMaster::pinConnected:

  case AudioMaster::wantMidi:
  case AudioMaster::processEvents:
    return 0;
  default:
    assert(false);
  }
}

static intptr_t callDispatcher(AEffect* aEffect, i32 opcode, i32 index, intptr_t value, void* ptr, f32 opt)
{
  return aEffect->dispatcher(aEffect, opcode, index, value, ptr, opt);
}

static std::string getString(AEffect* aEffect, i32 opcode, i32 index = 0)
{
  char buf[256]{};

  callDispatcher(aEffect, opcode, index, 0, buf, 0.0);

  return buf;
}

#define INT32_SWAP(val) \
   ((i32) ( \
    (((u32) (val) & (u32) 0x000000ffU) << 24) | \
    (((u32) (val) & (u32) 0x0000ff00U) <<  8) | \
    (((u32) (val) & (u32) 0x00ff0000U) >>  8) | \
    (((u32) (val) & (u32) 0xff000000U) >> 24)))

void Vst::init()
{
  for (const auto& file : std::filesystem::directory_iterator(std::filesystem::path("vst")))
  {
    if (file.path().extension() != std::filesystem::path(".dll"))
      continue;

    HINSTANCE hinstLib = LoadLibrary(TEXT(file.path().string().c_str()));
    assert(hinstLib != nullptr);

    vstPluginMain pluginMain = (vstPluginMain)GetProcAddress(hinstLib, "VSTPluginMain");
    if (pluginMain == nullptr)
      pluginMain = (vstPluginMain)GetProcAddress(hinstLib, "main");
    assert(pluginMain != nullptr);

    vstPlugins.push_back(VstPlugin());
    VstPlugin& vstPlugin = vstPlugins[vstPlugins.size() - 1];
    vstPlugin.aEffect = pluginMain(AudioMaster);
    vstPlugin.aEffect->ptr2 = &vstPlugin;

    callDispatcher(vstPlugin.aEffect, Eff::SetSampleRate, 0, 0, NULL, f32(Global::settings.audioSampleRate));
    callDispatcher(vstPlugin.aEffect, Eff::SetBlockSize, 0, Global::settings.audioBufferSize, NULL, 0);
    callDispatcher(vstPlugin.aEffect, Eff::Identify, 0, 0, NULL, 0);

    callDispatcher(vstPlugin.aEffect, Eff::Open, 0, 0, NULL, 0.0);

    vstPlugin.vstVersion = callDispatcher(vstPlugin.aEffect, Eff::GetVstVersion, 0, 0, NULL, 0);

    callDispatcher(vstPlugin.aEffect, Eff::SetSampleRate, 0, 0, NULL, 48000.0);
    callDispatcher(vstPlugin.aEffect, Eff::SetBlockSize, 0, 512, NULL, 0);

    if (vstPlugin.aEffect->magic == kEffectMagic &&
      !(vstPlugin.aEffect->flags & EffFlags::IsSynth) &&
      vstPlugin.aEffect->flags & EffFlags::CanReplacing)
    {
      if (vstPlugin.vstVersion >= 2)
      {
        vstPlugin.name = getString(vstPlugin.aEffect, Eff::GetEffectName);
        if (vstPlugin.name.length() == 0)
        {
          vstPlugin.name = getString(vstPlugin.aEffect, Eff::GetProductString);
        }
      }
      if (vstPlugin.name.length() == 0)
      {
        vstPlugin.name = file.path().stem().string();
      }

      if (vstPlugin.vstVersion >= 2)
      {
        vstPlugin.vendor = getString(vstPlugin.aEffect, Eff::GetVendorString);
        vstPlugin.version = INT32_SWAP(callDispatcher(vstPlugin.aEffect, Eff::GetVendorVersion, 0, 0, NULL, 0));
      }
      if (vstPlugin.version == 0)
      {
        vstPlugin.version = INT32_SWAP(vstPlugin.aEffect->version);
      }

      if (vstPlugin.aEffect->flags & EffFlags::HasEditor || vstPlugin.aEffect->numParams != 0)
      {
        vstPlugin.interactive = true;
      }

      vstPlugin.audioIns = vstPlugin.aEffect->numInputs;
      vstPlugin.audioOuts = vstPlugin.aEffect->numOutputs;

      vstPlugin.midiIns = 0;
      vstPlugin.midiOuts = 0;

      vstPlugin.automatable = false;
      for (i32 i = 0; i < vstPlugin.aEffect->numParams; i++)
      {
        if (callDispatcher(vstPlugin.aEffect, Eff::CanBeAutomated, 0, i, NULL, 0.0))
        {
          vstPlugin.automatable = true;
          break;
        }
      }
    }
  }
}

struct VstRect
{
  i16 top;
  i16 left;
  i16 bottom;
  i16 right;
};

void* Vst::openWindow(i32 index)
{
  assert(index >= 0);
  assert(index < vstPlugins.size());

  VstPlugin& vstPlugin = vstPlugins[index];

  VstRect rect;
  callDispatcher(vstPlugin.aEffect, Eff::EditGetRect, 0, 0, &rect, 0.0);

  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  SDL_GetWindowWMInfo(Global::window, &wmInfo);
  HWND hwnd = wmInfo.info.win.window;

  callDispatcher(vstPlugin.aEffect, Eff::EditOpen, 0, 0, hwnd, 0.0);

  HWND vstHwnd = GetWindow(hwnd, GW_CHILD);

  return vstHwnd;
}

void Vst::moveWindow(void* hwnd, i32 x, i32 y)
{
  MoveWindow((HWND)hwnd, x, y, 892, 210, TRUE);
}

void Vst::closeWindow(void* hwnd)
{
  DestroyWindow((HWND)hwnd);
}

#endif // SUPPORT_VST