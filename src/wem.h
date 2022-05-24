#ifndef WEM_H
#define WEM_H

#include "typedefs.h"

#include <vector>

namespace Wem
{
  std::vector<u8> to_ogg(const u8* wemData, u64 wemDataSize);
}

#endif // WEM_H