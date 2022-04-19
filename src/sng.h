#ifndef SNG_H
#define SNG_H

#include "typedefs.h"
#include <vector>

namespace Sng {
  struct Info {
    std::string name;
  };

  Sng::Info parse(const std::vector<u8>& sngData);
}

#endif // SNG_H
