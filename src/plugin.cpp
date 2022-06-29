#include "plugin.h"

#include "global.h"

#ifdef SUPPORT_VST
#include "vst.h"
#endif // SUPPORT_VST

#ifdef SUPPORT_VST3
#include "vst3.h"
#endif // SUPPORT_VST3

#ifdef SUPPORT_VST3
static i32 vst3IndexOffset;
#endif // SUPPORT_VST3

enum struct PluginType
{
  vst,
  vst3
};

static PluginType pluginType(i32 index)
{
#ifdef SUPPORT_VST3
  if (index >= vst3IndexOffset)
    return PluginType::vst3;
#endif // SUPPORT_VST3

#ifdef SUPPORT_VST
  return PluginType::vst;
#endif // SUPPORT_VST
}

void Plugin::init()
{
#ifdef SUPPORT_VST
  Vst::init();
#endif // SUPPORT_VST

#ifdef SUPPORT_VST3
  vst3IndexOffset = Global::pluginNames.size();
  Vst3::init();
#endif // SUPPORT_VST3
}

void Plugin::openWindow(i32 index, i32 instance)
{
  assert(index >= 0);

  switch (pluginType(index))
  {
#ifdef SUPPORT_VST
  case PluginType::vst:
    Vst::openWindow(index, instance);
    return;
#endif // SUPPORT_VST

#ifdef SUPPORT_VST3
  case PluginType::vst3:
    Vst3::openWindow(index - vst3IndexOffset, instance);
    return;
#endif // SUPPORT_VST3
  }
}

Rect Plugin::getWindowRect(i32 index)
{
  assert(index >= 0);

  switch (pluginType(index))
  {
#ifdef SUPPORT_VST
  case PluginType::vst:
    return Vst::getWindowRect(index);
#endif // SUPPORT_VST

#ifdef SUPPORT_VST3
  case PluginType::vst3:
    return Vst3::getWindowRect(index - vst3IndexOffset);
#endif // SUPPORT_VST3
  }
}

void Plugin::moveWindow(i32 index, i32 x, i32 y)
{
  assert(index >= 0);

  switch (pluginType(index))
  {
#ifdef SUPPORT_VST
  case PluginType::vst:
    Vst::moveWindow(index, x, y);
    return;
#endif // SUPPORT_VST

#ifdef SUPPORT_VST3
  case PluginType::vst3:
    Vst3::moveWindow(index - vst3IndexOffset, x, y);
    return;
#endif // SUPPORT_VST3
  }
}
void Plugin::closeWindow(i32 index)
{
  assert(index >= 0);

  switch (pluginType(index))
  {
#ifdef SUPPORT_VST
  case PluginType::vst:
    Vst::closeWindow(index);
    return;
#endif // SUPPORT_VST

#ifdef SUPPORT_VST3
  case PluginType::vst3:
    Vst3::closeWindow(index - vst3IndexOffset);
    return;
#endif // SUPPORT_VST3
  }
}

u64 Plugin::processBlock(i32 index, i32 instance, f32** inBlock, f32** outBlock, i32 blockLen)
{
  assert(index >= 0);

  switch (pluginType(index))
  {
#ifdef SUPPORT_VST
  case PluginType::vst:
    return Vst::processBlock(index, instance, inBlock, outBlock, blockLen);
#endif // SUPPORT_VST

#ifdef SUPPORT_VST3
  case PluginType::vst3:
    return Vst3::processBlock(index - vst3IndexOffset, instance, inBlock, outBlock, blockLen);
#endif // SUPPORT_VST3
  }
}

std::string Plugin::saveParameters(i32 index, i32 instance)
{
  assert(index >= 0);

  switch (pluginType(index))
  {
#ifdef SUPPORT_VST
  case PluginType::vst:
    return Vst::saveParameters(index, instance);
#endif // SUPPORT_VST

#ifdef SUPPORT_VST3
  case PluginType::vst3:
    return Vst3::saveParameters(index - vst3IndexOffset, instance);
#endif // SUPPORT_VST3
  }
}
void Plugin::loadParameter(i32 index, i32 instance, const std::string& base64)
{
  assert(index >= 0);

  switch (pluginType(index))
  {
#ifdef SUPPORT_VST
  case PluginType::vst:
    Vst::loadParameter(index, instance, base64);
    return;
#endif // SUPPORT_VST

#ifdef SUPPORT_VST3
  case PluginType::vst3:
    Vst3::loadParameter(index - vst3IndexOffset, instance, base64);
    return;
#endif // SUPPORT_VST3
  }
}