#ifndef PROFILE_H
#define PROFILE_H

#include "configuration.h"
#include "global.h"

namespace Profile {

  void init();
  void tick();
  void fini();

  void saveTone();
};

#endif // PROFILE_H