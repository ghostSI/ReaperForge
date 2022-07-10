#include "bnk.h"

#ifdef SUPPORT_BNK

#include "global.h"

#ifdef BNK_DLL_IMPORT // enabled use __declspec(dllimport), disabled use LoadLibrary

extern "C" __declspec(dllimport) void bnkInit(const char*, void*);
extern "C" __declspec(dllimport) void bnkTick();
extern "C" __declspec(dllimport) i32 bnkLoadBank(const char*, u32* const);
extern "C" __declspec(dllimport) i32 bnkPostEvent(i32);
extern "C" __declspec(dllimport) i32 bnkSetRTPCValue(i32, f32);
extern "C" __declspec(dllimport) void bnkInputOn(i32);

#else // BNK_DLL_IMPORT
#include <windows.h> 

typedef void (*bnkInitFunc)(const char*, void*);
static bnkInitFunc bnkInit;

typedef void (*bnkTickFunc)();
static bnkTickFunc bnkTick;

typedef u32(*bnkLoadBankFunc)(const char*, u32* const);
static bnkLoadBankFunc bnkLoadBank;

typedef u32(*bnkPostEventFunc)(u32);
static bnkPostEventFunc bnkPostEvent;

typedef u32(*bnkSetRTPCValueFunc)(u32, f32);
static bnkSetRTPCValueFunc bnkSetRTPCValue;

typedef u32(*bnkInputOnFunc)(u32);
static bnkInputOnFunc bnkInputOn;

static bool loadLibrary()
{
#ifdef UNICODE
  HINSTANCE hinstLib = LoadLibrary(L"bnkPlugin.dll");
#else // UNICODE
  HINSTANCE hinstLib = LoadLibrary("bnkPlugin.dll");
#endif // UNICODE
  if (hinstLib)
  {
    bnkInit = (bnkInitFunc)GetProcAddress(hinstLib, "bnkInit");
    assert(bnkInit != nullptr);
    bnkTick = (bnkTickFunc)GetProcAddress(hinstLib, "bnkTick");
    assert(bnkTick != nullptr);
    bnkLoadBank = (bnkLoadBankFunc)GetProcAddress(hinstLib, "bnkLoadBank");
    assert(bnkLoadBank != nullptr);
    bnkPostEvent = (bnkPostEventFunc)GetProcAddress(hinstLib, "bnkPostEvent");
    assert(bnkPostEvent != nullptr);
    bnkSetRTPCValue = (bnkSetRTPCValueFunc)GetProcAddress(hinstLib, "bnkSetRTPCValue");
    assert(bnkSetRTPCValue != nullptr);
    bnkInputOn = (bnkInputOnFunc)GetProcAddress(hinstLib, "bnkInputOn");
    assert(bnkInputOn != nullptr);

    return true;
  }
  return false;
}
#endif // BNK_DLL_IMPORT

void Bnk::init()
{
  if (Global::settings.audioSignalChain != SignalChain::soundBank)
    return;

#ifdef BNK_DLL_IMPORT
  Global::bnkPluginLoaded = true;
#else
  Global::bnkPluginLoaded = loadLibrary();
#endif // BNK_DLL_IMPORT

  if (!Global::bnkPluginLoaded)
    return;

  bnkInit(Global::settings.bnkPath.c_str(), Global::hWnd);
}

void Bnk::tick()
{
  if (!Global::bnkPluginLoaded)
    return;

  bnkTick();
}

Bnk::Result Bnk::loadBank(const char* name, Bnk::BankID& bankID)
{
  const Bnk::Result result = Bnk::Result(bnkLoadBank(name, &bankID));
  //assert(result == Bnk::Result::Success);
  return result;
}

Bnk::PlayID Bnk::postEvent(Bnk::UniqueID eventID)
{
  return bnkPostEvent(eventID);
}

Bnk::Result Bnk::setRTPCValue(Bnk::RtpcID rtpcId, Bnk::RtpcValue value)
{
  const Bnk::Result result = Bnk::Result(bnkSetRTPCValue(rtpcId, value));
  //assert(result == Bnk::Result::Success);
  return result;
}

void Bnk::inputOn(Bnk::PlayID playID)
{
  if (Global::settings.audioSignalChain != SignalChain::soundBank)
    return;

  bnkInputOn(playID);
}



//int main(int argc, char* argv[])
//{
//  bnkInit();
//
//  const std::vector<std::string> bnkFileNames =
//  {
//#include "../../bnk/bnkFileNames.txt"
//  };
//
//  std::vector<BnkResult> results;
//  for (const std::string& bnk : bnkFileNames)
//  {
//    BnkBankID bankID;
//    results.push_back(bnkLoadBank(bnk.c_str(), bankID));
//  }
//
//  std::vector<std::string> success;
//  std::vector<std::string> fail;
//  std::vector<std::string> wrongBankVersion;
//  std::vector<std::string> bankAlreadyLoaded;
//  std::vector<std::string> insufficientMemory;
//  for (int i = 0; i < results.size(); ++i)
//  {
//    switch (results[i])
//    {
//    case BnkResult::Success:
//      success.push_back(bnkFileNames[i]);
//      break;
//    case BnkResult::Fail:
//      fail.push_back(bnkFileNames[i]);
//      break;
//    case BnkResult::WrongBankVersion:
//      wrongBankVersion.push_back(bnkFileNames[i]);
//      break;
//    case BnkResult::BankAlreadyLoaded:
//      bankAlreadyLoaded.push_back(bnkFileNames[i]);
//      break;
//    case BnkResult::InsufficientMemory:
//      insufficientMemory.push_back(bnkFileNames[i]);
//      break;
//    default:
//      assert(false);
//      break;
//    }
//  }
//
//  assert(183 == success.size());
//  assert(0 == fail.size());
//  assert(0 == wrongBankVersion.size());
//  assert(0 == bankAlreadyLoaded.size());
//  assert(38 == insufficientMemory.size());
//
//  return 0;
//}

#endif // SUPPORT_BNK
