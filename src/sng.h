#ifndef SNG_H
#define SNG_H

#include "psarc.h"

namespace Sng {
  struct Info {
    std::string name;
  };

  Sng::Info parse(const Psarc::Info::TOCEntry& tocEntry);
}

#endif // SNG_H
