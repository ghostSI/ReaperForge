#ifndef VST_H
#define VST_H

#include "configuration.h"

#ifdef SUPPORT_VST

#include "type.h"

#include <string>

namespace Vst
{
  void init();

  void openWindow(i32 index);
  Rect getWindowRect(i32 index);
  void moveWindow(i32 index, i32 x, i32 y);
  void closeWindow(i32 index);

  u64 processBlock(i32 index, f32** inBlock, f32** outBlock, size_t blockLen);

  std::string saveParameters(i32 index);
}

#endif // SUPPORT_VST

#endif // VST_H
