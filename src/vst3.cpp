#include "vst3.h"

#ifdef SUPPORT_VST3

#include "base64.h"
#include "global.h"
#include "version.h"

#include <SDL2/SDL_syswm.h>

#include <pluginterfaces/gui/iplugview.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <public.sdk/source/vst/hosting/hostclasses.h>
#include <public.sdk/source/vst/hosting/module.h>

#include <filesystem>

struct Vst3Plugin
{
  std::shared_ptr<VST3::Hosting::Module> module;
  std::vector<VST3::Hosting::ClassInfo> classInfos;

  Steinberg::Vst::ProcessSetup processSetup;

  Steinberg::IPtr<Steinberg::Vst::IComponent> effectComponent;
  Steinberg::IPtr<Steinberg::Vst::IEditController> editController;
  Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> audioProcessor;
  Steinberg::IPlugView* plugView{};
};

static std::vector<Vst3Plugin> vst3Plugins;

static void loadPluginInstance(Vst3Plugin& vst3Plugin)
{
  assert(vst3Plugin.editController);

  const i32 parameterCount = vst3Plugin.editController->getParameterCount();
  for (i32 i = 0; i < parameterCount; ++i)
  {
    Steinberg::Vst::ParameterInfo parameterInfo{};
    const Steinberg::tresult result = vst3Plugin.editController->getParameterInfo(i, parameterInfo);
    assert(result == Steinberg::kResultOk);

    assert(!(parameterInfo.flags & Steinberg::Vst::ParameterInfo::kIsReadOnly));

    const double value = vst3Plugin.editController->getParamNormalized(parameterInfo.id);
  }
}

#define INT32_SWAP(val) \
   ((i32) ( \
    (((u32) (val) & (u32) 0x000000ffU) << 24) | \
    (((u32) (val) & (u32) 0x0000ff00U) <<  8) | \
    (((u32) (val) & (u32) 0x00ff0000U) >>  8) | \
    (((u32) (val) & (u32) 0xff000000U) >> 24)))

static Steinberg::Vst::HostApplication context;

static void activateBuses(Steinberg::Vst::IComponent* effectComponent)
{
  {
    const i32 inputCount = effectComponent->getBusCount(Steinberg::Vst::kAudio, Steinberg::Vst::kInput);
    for (i32 i = 0; i < inputCount; ++i)
    {
      Steinberg::Vst::BusInfo busInfo;
      const Steinberg::tresult result = effectComponent->getBusInfo(Steinberg::Vst::kAudio, Steinberg::Vst::kInput, i, busInfo);
      assert(result == Steinberg::kResultOk);
      if (busInfo.flags & Steinberg::Vst::BusInfo::kDefaultActive)
        effectComponent->activateBus(Steinberg::Vst::kAudio, Steinberg::Vst::kInput, i, true);
    }
  }

  {
    const i32 outputCount = effectComponent->getBusCount(Steinberg::Vst::kAudio, Steinberg::Vst::kOutput);
    for (i32 i = 0; i < outputCount; ++i)
    {
      Steinberg::Vst::BusInfo busInfo;
      const Steinberg::tresult result = effectComponent->getBusInfo(Steinberg::Vst::kAudio, Steinberg::Vst::kOutput, i, busInfo);
      assert(result == Steinberg::kResultOk);
      if (busInfo.flags & Steinberg::Vst::BusInfo::kDefaultActive)
        effectComponent->activateBus(Steinberg::Vst::kAudio, Steinberg::Vst::kOutput, i, true);
    }
  }
}


void Vst3::init()
{
  const std::vector<std::string> paths = string::split(Global::settings.vst3Path, ';');
  for (const std::string& path : paths)
  {
    for (const auto& file : std::filesystem::directory_iterator(std::filesystem::path(Global::settings.vst3Path)))
    {
      if (file.path().extension() != std::filesystem::path(".vst3"))
        continue;

      vst3Plugins.push_back(Vst3Plugin());
      Vst3Plugin& vst3Plugin = vst3Plugins[vst3Plugins.size() - 1];
      vst3Plugin.processSetup.processMode = Steinberg::Vst::kRealtime;
      vst3Plugin.processSetup.symbolicSampleSize = Steinberg::Vst::kSample32;
      vst3Plugin.processSetup.maxSamplesPerBlock = 8192;
      vst3Plugin.processSetup.sampleRate = f64(Global::settings.audioSampleRate);

      {
        std::string error;
        vst3Plugin.module = VST3::Hosting::Module::create(file.path().string(), error);
        assert(error.size() == 0);
      }

      const VST3::Hosting::PluginFactory& pluginFactory = vst3Plugin.module->getFactory();
      const u64 classInfoCount = pluginFactory.classInfos().size();
      for (u64 i = 0; i < classInfoCount; ++i)
      {
        vst3Plugin.classInfos = pluginFactory.classInfos();
        const VST3::Hosting::ClassInfo& classInfo = vst3Plugin.classInfos[i];
        Global::pluginNames.push_back(classInfo.name());
        Global::vst3PluginNames.push_back(classInfo.name());

        if (i != 0)
          continue;

        vst3Plugin.effectComponent = pluginFactory.createInstance<Steinberg::Vst::IComponent>(classInfo.ID());
        assert(vst3Plugin.effectComponent != nullptr);

        const Steinberg::tresult result = vst3Plugin.effectComponent->initialize(&context);
        assert(result == Steinberg::kResultOk);

        vst3Plugin.audioProcessor = Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor>(vst3Plugin.effectComponent);
        assert(vst3Plugin.audioProcessor != nullptr);

        activateBuses(vst3Plugin.effectComponent);

        vst3Plugin.editController = Steinberg::FUnknownPtr<Steinberg::Vst::IEditController>(vst3Plugin.effectComponent);

        if (!vst3Plugin.editController)
        {
          Steinberg::TUID controllerCID;
          {
            const Steinberg::tresult result = vst3Plugin.effectComponent->getControllerClassId(controllerCID);
            assert(result == Steinberg::kResultOk);
          }
          {
            vst3Plugin.editController = pluginFactory.createInstance<Steinberg::Vst::IEditController>(VST3::UID(controllerCID));
            assert(vst3Plugin.editController);
          }
          {
            const Steinberg::tresult result = vst3Plugin.editController->initialize(&context);
            assert(result == Steinberg::kResultOk);
          }
        }

        auto componentConnectionPoint = Steinberg::FUnknownPtr<Steinberg::Vst::IConnectionPoint>{ vst3Plugin.effectComponent };
        auto controllerConnectionPoint = Steinberg::FUnknownPtr<Steinberg::Vst::IConnectionPoint>{ vst3Plugin.editController };
      }
    }
  }

  //Vst3::openWindow(0, 0);
}

//class PlugFrame final : public Steinberg::IPlugFrame
//{
//  HWND parent;
//  PlugFrame(HWND parent) : parent(parent)
//  {
//    //FUNKNOWN_CTOR
//  }
//
//  Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView* view, Steinberg::ViewRect* newSize) override
//  {
//    return Steinberg::kResultOk;
//  }
//}

void Vst3::openWindow(i32 index, i32 instance)
{
  assert(index >= 0);
  assert(index < vst3Plugins.size());

  Vst3Plugin& vst3Plugin = vst3Plugins[index];
  assert(vst3Plugin.plugView == nullptr);
  assert(vst3Plugin.editController);

  vst3Plugin.plugView = vst3Plugin.editController->createView(Steinberg::Vst::ViewType::kEditor);

  assert(vst3Plugin.plugView != nullptr);

  {
    Steinberg::ViewRect defaultSize;
    const Steinberg::tresult result = vst3Plugin.plugView->getSize(&defaultSize);
    assert(result == Steinberg::kResultOk);
  }

  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  SDL_GetWindowWMInfo(Global::window, &wmInfo);

  //PlugFrame* plugFrame = new PlugFrame(wmInfo.info.win.window);
  //vst3Plugin.plugView->setFrame(plugFrame);

  {
    const Steinberg::tresult result = vst3Plugin.plugView->attached(wmInfo.info.win.window, Steinberg::kPlatformTypeHWND);
    assert(result == Steinberg::kResultOk);
  }
}

Rect Vst3::getWindowRect(i32 index)
{
  assert(index >= 0);
  assert(index < vst3Plugins.size());

  Vst3Plugin& vst3Plugin = vst3Plugins[index];
  assert(vst3Plugin.plugView != nullptr);

  Steinberg::ViewRect size;
  const Steinberg::tresult result = vst3Plugin.plugView->getSize(&size);
  assert(result == Steinberg::kResultOk);

  const Rect r = {
    .top = i16(size.top),
    .left = i16(size.left),
    .bottom = i16(size.bottom),
    .right = i16(size.right)
  };

  return r;
}

void Vst3::moveWindow(i32 index, i32 x, i32 y)
{
  assert(index >= 0);
  assert(index < vst3Plugins.size());

  //MoveWindow(vst3Plugins[index].hwnd, x, y, vst3Plugins[index].windowRect->right, vst3Plugins[index].windowRect->bottom, TRUE);
}

void Vst3::closeWindow(i32 index)
{
  assert(index >= 0);
  assert(index < vst3Plugins.size());

  //DestroyWindow(vst3Plugins[index].hwnd);
}


u64 Vst3::processBlock(i32 index, i32 instance, f32** inBlock, f32** outBlock, i32 blockLen)
{
  //assert(index >= 0);
  //assert(index < vst3Plugins.size());

  //if (blockLen >= 0)
  //  vst3Plugins[index].aEffect[instance]->processReplacing(vst3Plugins[index].aEffect[instance], inBlock, outBlock, blockLen);

  return blockLen;
}

std::string Vst3::saveParameters(i32 index, i32 instance)
{
  //assert(index >= 0);
  //assert(index < vst3Plugins.size());

  //u8* chunk = nullptr;
  //if (to_underlying(vst3Plugins[index].aEffect[instance]->flags & EffFlags::ProgramChunks))
  //{
  //  const i64 len = callDispatcher(vst3Plugins[index].aEffect[instance], EffOpcode::GetChunk, 1, 0, &chunk, 0.0);
  //  assert(len > 0);

  //  return Base64::encode(chunk, len);
  //}
  return {};
}

void Vst3::loadParameter(i32 index, i32 instance, const std::string& base64)
{
  //assert(index >= 0);
  //assert(index < vst3Plugins.size());
  //u8 data[4096];
  //const i64 len = Base64::decode(base64, data);
  //assert(len >= 0);

  //if (instance == vst3Plugins[index].aEffect.size())
  //  loadPluginInstance(vst3Plugins[index]); // plugin is loaded multiple times, create another instance

  //callDispatcher(vst3Plugins[index].aEffect[instance], EffOpcode::BeginSetProgram, 0, 0, nullptr, 0.0);
  //callDispatcher(vst3Plugins[index].aEffect[instance], EffOpcode::SetChunk, 1, len, data, 0.0);
  //callDispatcher(vst3Plugins[index].aEffect[instance], EffOpcode::EndSetProgram, 0, 0, nullptr, 0.0);
}

#endif // SUPPORT_VST