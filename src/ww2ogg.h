#ifndef WW2OGG_H
#define WW2OGG_H

#include "typedefs.h"

#include <vector>

namespace ww2ogg
{
  std::vector<u8> wemData2OggData(const u8* data, u64 size);
}

#endif // WW2OGG_H