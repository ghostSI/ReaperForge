#ifndef SOUND_H
#define SOUND_H

#include "typedefs.h"

namespace Sound
{
  void init();
  void fini();

  void pauseAudioDevice(bool pauseAudio);
};

#endif // SOUND_H
