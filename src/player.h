#ifndef PLAYER_H
#define PLAYER_H

#include "typedefs.h"

namespace Psarc { struct Info; }
enum struct InstrumentFlags : u16;

namespace Player {
  void tick();

  void playSong(const Psarc::Info& psarcInfo, InstrumentFlags instrumentFlags);
  void stop();
}

#endif // PLAYER_H
