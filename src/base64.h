#ifndef BASE64_H
#define BASE64_H

#include "typedefs.h"

#include <string>

namespace Base64
{
  std::string encode(const u8* in, u64 size);
  u64 decode(const std::string& in, u8* out);
}

#endif // BASE64_H
