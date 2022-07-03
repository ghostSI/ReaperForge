#include "vst.h"

#ifdef SUPPORT_VST

#include "base64.h"
#include "global.h"
#include "version.h"

#include <SDL2/SDL_syswm.h>

#include <windows.h> 

#include <filesystem>

enum struct AudioMasterOpcode : i32
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

enum struct EffFlags
{
  HasEditor = 1,
  HasClip = 2,
  HasVu = 4,
  CanMono = 8,
  CanReplacing = 1 << 4,
  ProgramChunks = 1 << 5,
  IsSynth = 1 << 8,
  NoSoundInStop = 1 << 11,
  ExtIsAsync = 1 << 10,
  ExtHasBuffer = 1 << 11,
  CanDoubleReplacing = 1 << 12
} BIT_FLAGS(EffFlags);

enum struct EffOpcode : i32
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
  GetVu = 9,
  SetSampleRate = 10,
  SetBlockSize = 11,
  MainsChanged = 12,
  EditGetRect = 13,
  EditOpen = 14,
  EditClose = 15,
  EditDraw = 16,
  EditMouse = 17,
  EditKey = 18,
  EditIdle = 19,
  EditTop = 20,
  EditSleep = 21,
  Identify = 22,
  GetChunk = 23,
  SetChunk = 24,
  ProcessEvents = 25,
  CanBeAutomated = 26,
  String2Parameter = 27,
  GetNumProgramCategories = 28,
  GetProgramNameIndexed = 29,
  CopyProgram = 30,
  ConnectInput = 31,
  ConnectOutput = 32,
  GetInputProperties = 33,
  GetOutputProperties = 34,
  GetPlugCategory = 35,
  GetCurrentPosition = 36,
  GetDestinationBuffer = 37,
  OfflineNotify = 38,
  OfflinePrepare = 39,
  OfflineRun = 40,
  ProcessVarIo = 41,
  SetSpeakerArrangement = 42,
  SetBlockSizeAndSampleRate = 43,
  SetBypass = 44,
  GetEffectName = 45,
  GetErrorText = 46,
  GetVendorString = 47,
  GetProductString = 48,
  GetVendorVersion = 49,
  VendorSpecific = 50,
  CanDo = 51,
  GetTailSize = 52,
  Idle = 53,
  GetIcon = 54,
  SetViewPosition = 55,
  GetParameterProperties = 56,
  KeysRequired = 57,
  GetVstVersion = 58,
  EditKeyDown = 59,
  EditKeyUp = 60,
  SetEditKnobMode = 61,
  GetMidiProgramName = 62,
  GetCurrentMidiProgram = 63,
  GetMidiProgramCategory = 64,
  HasMidiProgramsChanged = 65,
  GetMidiKeyName = 66,
  BeginSetProgram = 67,
  EndSetProgram = 68,
  GetSpeakerArrangement = 69,
  ShellGetNextPlugin = 70,
  StartProcess = 71,
  StopProcess = 72,
  SetTotalSampleToProcess = 73,
  SetPanLaw = 74,
  BeginLoadBank = 75,
  BeginLoadProgram = 76,
  SetProcessPrecision = 77,
  GetNumMidiInputChannels = 78,
  GetNumMidiOutputChannels = 79,
  NumOpcodes = 80
};

struct AEffect
{
  i32 magic;
  intptr_t(*dispatcher)(AEffect*, EffOpcode, i32, intptr_t, void*, f32);
  void (*process)(AEffect*, f32**, f32**, i32);
  void (*setParameter)(AEffect*, i32, f32);
  f32(*getParameter)(AEffect*, i32);
  i32 numPrograms;
  i32 numParams;
  i32 numInputs;
  i32 numOutputs;
  EffFlags flags;
  void* ptr1;
  void* ptr2;
  i32 initialDelay;
  i32 empty3a;
  i32 empty3b;
  f32 unkown_float;
  void* ptr3;
  void* user;
  i32 uniqueID;
  i32 version;
  void (*processReplacing)(AEffect*, f32**, f32**, i32);
};

typedef intptr_t(*audioMasterCallback)(AEffect*, AudioMasterOpcode, int32_t, intptr_t, void*, f32);
typedef AEffect* (*vstPluginMain)(audioMasterCallback audioMaster);

struct VstPlugin
{
  std::vector<AEffect*> aEffect; // allow multiple instances
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

  HWND hwnd;
  Rect* windowRect;
  vstPluginMain pluginMain;
};

static std::vector<VstPlugin> vstPlugins;


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

static intptr_t AudioMaster(AEffect* effect, AudioMasterOpcode opcode, int32_t index, intptr_t value, void* ptr, f32 opt)
{
  VstPlugin* vst = (effect ? (VstPlugin*)effect->ptr2 : nullptr);

  switch (opcode)
  {
  case AudioMasterOpcode::version:
    return (intptr_t)2400;

  case AudioMasterOpcode::currentId:
    return 0;

  case AudioMasterOpcode::getVendorString:
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
    strcpy((char*)ptr, "ReaperForge");
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32
    return 1;

  case AudioMasterOpcode::getProductString:
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
    strcpy((char*)ptr, "ReaperForge");
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32
    return 1;

  case AudioMasterOpcode::getVendorVersion:
    return (intptr_t)(VERSION_MAJOR << 24 | VERSION_MINOR << 16 | VERSION_PATCH << 8 | VERSION_BUILD);

  case AudioMasterOpcode::needIdle:
    return 0;

  case AudioMasterOpcode::updateDisplay:
    return 0;

  case AudioMasterOpcode::getTime:
    return 0;

  case AudioMasterOpcode::ioChanged:
    return 0;

  case AudioMasterOpcode::getSampleRate:
    return (intptr_t)Global::settings.audioSampleRate;

  case AudioMasterOpcode::idle:
    return 1;

  case AudioMasterOpcode::getCurrentProcessLevel:
    return 0;

  case AudioMasterOpcode::getLanguage:
    return kVstLangEnglish;

  case AudioMasterOpcode::willReplaceOrAccumulate:
    return 1;

  case AudioMasterOpcode::sizeWindow:
    return 1;

  case AudioMasterOpcode::canDo:
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

  case AudioMasterOpcode::beginEdit:
  case AudioMasterOpcode::endEdit:
    return 0;

  case AudioMasterOpcode::automate:
    return 0;

  case AudioMasterOpcode::pinConnected:

  case AudioMasterOpcode::wantMidi:
  case AudioMasterOpcode::processEvents:
    return 0;
  }

  //assert(false);
  return 0;
}

static intptr_t callDispatcher(AEffect* aEffect, EffOpcode opcode, i32 index, intptr_t value, void* ptr, f32 opt)
{
  return aEffect->dispatcher(aEffect, opcode, index, value, ptr, opt);
}

static std::string getString(AEffect* aEffect, EffOpcode opcode, i32 index = 0)
{
  char buf[256]{};

  callDispatcher(aEffect, opcode, index, 0, buf, 0.0);

  return buf;
}

static void loadPluginInstance(VstPlugin& vstPlugin)
{
  const i32 instance = i32(vstPlugin.aEffect.size());
  vstPlugin.aEffect.push_back(vstPlugin.pluginMain(AudioMaster));
  vstPlugin.aEffect[instance]->ptr2 = &vstPlugin;

  callDispatcher(vstPlugin.aEffect[instance], EffOpcode::SetSampleRate, 0, 0, nullptr, f32(Global::settings.audioSampleRate));
  callDispatcher(vstPlugin.aEffect[instance], EffOpcode::SetBlockSize, 0, Global::settings.audioBufferSize, nullptr, 0);
  callDispatcher(vstPlugin.aEffect[instance], EffOpcode::Identify, 0, 0, nullptr, 0);

  callDispatcher(vstPlugin.aEffect[instance], EffOpcode::Open, 0, 0, nullptr, 0.0);

  callDispatcher(vstPlugin.aEffect[instance], EffOpcode::BeginSetProgram, 0, 0, nullptr, 0.0);
  callDispatcher(vstPlugin.aEffect[instance], EffOpcode::SetProgram, 0, 0, nullptr, 0.0);
  callDispatcher(vstPlugin.aEffect[instance], EffOpcode::EndSetProgram, 0, 0, nullptr, 0.0);

  callDispatcher(vstPlugin.aEffect[instance], EffOpcode::MainsChanged, 0, 1, nullptr, 0.0);

  const i32 vstVersion = i32(callDispatcher(vstPlugin.aEffect[instance], EffOpcode::GetVstVersion, 0, 0, nullptr, 0)); // might not be needed
  if (vstVersion >= 2)
    callDispatcher(vstPlugin.aEffect[instance], EffOpcode::StartProcess, 0, 0, nullptr, 0.0);

  callDispatcher(vstPlugin.aEffect[instance], EffOpcode::SetSampleRate, 0, 0, nullptr, f32(Global::settings.audioSampleRate));
  callDispatcher(vstPlugin.aEffect[instance], EffOpcode::SetBlockSize, 0, Global::settings.audioBufferSize, nullptr, 0.0);
}

#define INT32_SWAP(val) \
   ((i32) ( \
    (((u32) (val) & (u32) 0x000000ffU) << 24) | \
    (((u32) (val) & (u32) 0x0000ff00U) <<  8) | \
    (((u32) (val) & (u32) 0x00ff0000U) >>  8) | \
    (((u32) (val) & (u32) 0xff000000U) >> 24)))

void Vst::init()
{
  const std::vector<std::string> paths = string::split(Global::settings.vstPath, ';');
  for (const std::string& path : paths)
  {
    for (const auto& file : std::filesystem::directory_iterator(std::filesystem::path(path)))
    {
      if (file.path().extension() != std::filesystem::path(".dll"))
        continue;

#ifdef UNICODE
      HINSTANCE hinstLib = LoadLibrary(file.path().wstring().c_str());
#else // UNICODE
      HINSTANCE hinstLib = LoadLibrary(file.path().string().c_str());
#endif // UNICODE

      assert(hinstLib != nullptr);

      vstPlugins.push_back(VstPlugin());
      VstPlugin& vstPlugin = vstPlugins[vstPlugins.size() - 1];

      vstPlugin.pluginMain = (vstPluginMain)GetProcAddress(hinstLib, "VSTPluginMain");
      if (vstPlugin.pluginMain == nullptr)
        vstPlugin.pluginMain = (vstPluginMain)GetProcAddress(hinstLib, "main");
      assert(vstPlugin.pluginMain != nullptr);

      loadPluginInstance(vstPlugin);

      vstPlugin.vstVersion = i32(callDispatcher(vstPlugin.aEffect[0], EffOpcode::GetVstVersion, 0, 0, nullptr, 0));

      if (vstPlugin.aEffect[0]->magic == kEffectMagic &&
        !(to_underlying(vstPlugin.aEffect[0]->flags & EffFlags::IsSynth)) &&
        to_underlying(vstPlugin.aEffect[0]->flags & EffFlags::CanReplacing))
      {
        if (vstPlugin.vstVersion >= 2)
        {
          vstPlugin.name = getString(vstPlugin.aEffect[0], EffOpcode::GetEffectName);
          if (vstPlugin.name.length() == 0)
          {
            vstPlugin.name = getString(vstPlugin.aEffect[0], EffOpcode::GetProductString);
          }
        }
        if (vstPlugin.name.length() == 0)
        {
          vstPlugin.name = file.path().stem().string();
        }

        if (vstPlugin.vstVersion >= 2)
        {
          vstPlugin.vendor = getString(vstPlugin.aEffect[0], EffOpcode::GetVendorString);
          vstPlugin.version = INT32_SWAP(callDispatcher(vstPlugin.aEffect[0], EffOpcode::GetVendorVersion, 0, 0, nullptr, 0));
        }
        if (vstPlugin.version == 0)
        {
          vstPlugin.version = INT32_SWAP(vstPlugin.aEffect[0]->version);
        }

        if (to_underlying(vstPlugin.aEffect[0]->flags & EffFlags::HasEditor) || vstPlugin.aEffect[0]->numParams != 0)
        {
          vstPlugin.interactive = true;
        }

        vstPlugin.audioIns = vstPlugin.aEffect[0]->numInputs;
        vstPlugin.audioOuts = vstPlugin.aEffect[0]->numOutputs;

        vstPlugin.midiIns = 0;
        vstPlugin.midiOuts = 0;

        vstPlugin.automatable = false;
        for (i32 i = 0; i < vstPlugin.aEffect[0]->numParams; i++)
        {
          if (callDispatcher(vstPlugin.aEffect[0], EffOpcode::CanBeAutomated, 0, i, nullptr, 0.0))
          {
            vstPlugin.automatable = true;
            break;
          }
        }
      }
      Global::pluginNames.push_back("VST " + vstPlugin.name);
    }
  }
}

void Vst::openWindow(i32 index, i32 instance)
{
  assert(index >= 0);
  assert(index < vstPlugins.size());

  VstPlugin& vstPlugin = vstPlugins[index];

  callDispatcher(vstPlugin.aEffect[instance], EffOpcode::EditGetRect, 0, 0, &vstPlugin.windowRect, 0.0);
  assert(vstPlugin.windowRect->top == 0);
  assert(vstPlugin.windowRect->left == 0);
  assert(vstPlugin.windowRect->bottom >= 1);
  assert(vstPlugin.windowRect->right >= 1);

  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  SDL_GetWindowWMInfo(Global::window, &wmInfo);

  callDispatcher(vstPlugin.aEffect[instance], EffOpcode::EditOpen, 0, 0, wmInfo.info.win.window, 0.0);

  vstPlugin.hwnd = GetWindow(wmInfo.info.win.window, GW_CHILD);
}

Rect Vst::getWindowRect(i32 index)
{
  assert(index >= 0);
  assert(index < vstPlugins.size());

  return *vstPlugins[index].windowRect;
}

void Vst::moveWindow(i32 index, i32 x, i32 y)
{
  assert(index >= 0);
  assert(index < vstPlugins.size());

  MoveWindow(vstPlugins[index].hwnd, x, y, vstPlugins[index].windowRect->right, vstPlugins[index].windowRect->bottom, TRUE);
}

void Vst::closeWindow(i32 index)
{
  assert(index >= 0);
  assert(index < vstPlugins.size());

  DestroyWindow(vstPlugins[index].hwnd);
}


u64 Vst::processBlock(i32 index, i32 instance, f32** inBlock, f32** outBlock, i32 blockLen)
{
  assert(index >= 0);
  assert(index < vstPlugins.size());

  // After the start of the application one instance is created for each plugin.
  // When a plugin will be used twice or more in a effect chain an additional instance is needed.
  // Instead of pausing the audio, creating an new instance, unpausing the audio. It is done here.
  // Right now there is no way to pause the audio output. It might need to be changed if this does not work as expected.
  while (vstPlugins[index].aEffect.size() <= instance)
    loadPluginInstance(vstPlugins[index]);

  if (blockLen >= 0)
    vstPlugins[index].aEffect[instance]->processReplacing(vstPlugins[index].aEffect[instance], inBlock, outBlock, blockLen);

  return blockLen;
}

std::string Vst::saveParameters(i32 index, i32 instance)
{
  assert(index >= 0);
  assert(index < vstPlugins.size());

  u8* chunk = nullptr;
  if (to_underlying(vstPlugins[index].aEffect[instance]->flags & EffFlags::ProgramChunks))
  {
    const i64 len = callDispatcher(vstPlugins[index].aEffect[instance], EffOpcode::GetChunk, 1, 0, &chunk, 0.0);
    //assert(len > 0);

    return Base64::encode(chunk, len);
  }
  return {};
}

void Vst::loadParameter(i32 index, i32 instance, const std::string& base64)
{
  assert(index >= 0);
  assert(index < vstPlugins.size());
  u8 data[4096];
  const i64 len = Base64::decode(base64, data);
  assert(len >= 0);

  if (instance == vstPlugins[index].aEffect.size())
    loadPluginInstance(vstPlugins[index]); // plugin is loaded multiple times, create another instance

  callDispatcher(vstPlugins[index].aEffect[instance], EffOpcode::BeginSetProgram, 0, 0, nullptr, 0.0);
  callDispatcher(vstPlugins[index].aEffect[instance], EffOpcode::SetChunk, 1, len, data, 0.0);
  callDispatcher(vstPlugins[index].aEffect[instance], EffOpcode::EndSetProgram, 0, 0, nullptr, 0.0);
}

#endif // SUPPORT_VST