#ifndef SAVES_H
#define SAVES_H

#include "manifest.h"

namespace Saves {

  void save();
  std::vector<Manifest::Info> load();
};

#endif // SAVES_H