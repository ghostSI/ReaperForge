#ifndef VST3_H
#define VST3_H

#include "configuration.h"

#ifdef SUPPORT_VST3

#include "type.h"

#include <string>

namespace Vst3
{
  void init();

  void openWindow(i32 index, i32 instance);
  Rect getWindowRect(i32 index);
  void moveWindow(i32 index, i32 x, i32 y);
  void closeWindow(i32 index);

  u64 processBlock(i32 index, i32 instance, f32** inBlock, f32** outBlock, i32 blockLen);

  std::string saveParameters(i32 index, i32 instance);
  void loadParameter(i32 index, i32 instance, const std::string& base64);
}

#endif // SUPPORT_VST3

#endif // VST3_H
