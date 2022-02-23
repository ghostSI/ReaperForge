#ifndef ZORDER_H
#define ZORDER_H

#include "typedefs.h"

enum struct ZOrder : i32
{
  background0,
  hud0,
  fontWorld,
  ui,
  fontUi,
  zOrderNotSpecified,
};

f32 zOrder(ZOrder order);

#endif // ZORDER_H
