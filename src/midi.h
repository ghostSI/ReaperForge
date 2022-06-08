#ifndef MIDI_H
#define MIDI_H

#include "configuration.h"

#ifdef SUPPORT_MIDI

#include "type.h"

#include <vector>

namespace Midi
{
  void init();

  void openDevice(i32 index);
}

#endif // SUPPORT_MIDI

#endif // MIDI_H
