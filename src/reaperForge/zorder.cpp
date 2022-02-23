#include "zorder.h"

f32 zOrder(ZOrder order)
{
  static f32 value[] = {
    0.99_f32, // background0
    0.51_f32, // hud0
    0.32_f32, // ui
    0.30_f32, // fontUi
    0.29_f32, // zOrderNotSpecified
  };

  return value[to_underlying(order)];
}
