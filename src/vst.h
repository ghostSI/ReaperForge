#ifndef VST_H
#define VST_H

#include "configuration.h"

#ifdef SUPPORT_VST

#include "typedefs.h"

#include <string>
#include <vector>

namespace Vst
{
  void init();
  void* openWindow(i32 index);
  void moveWindow(void* hwnd, i32 x, i32 y);
  void closeWindow(void* hwnd);
}

#endif // SUPPORT_VST

#endif // VST_H
