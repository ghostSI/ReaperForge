#ifndef PROFILE_H
#define PROFILE_H

#include "configuration.h"
#include "global.h"

namespace Profile {

  void init();
  void tick();
  void fini();

#ifdef SUPPORT_PLUGIN
  void saveTone();
#endif // SUPPORT_PLUGIN
};

#endif // PROFILE_H