#ifndef PLAYER_H
#define PLAYER_H

namespace Psarc { struct Info; }

namespace Player {
  void tick();

  void playSong(const Psarc::Info& psarcInfo);
  void stop();
}

#endif // PLAYER_H
