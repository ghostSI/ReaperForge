#ifndef PCM_H
#define PCM_H

#include "typedefs.h"

namespace Pcm {
  i32 decodeOgg(const u8* oggData, u32 oggDataSize, u8** pcmData, u32& pcmDataSize);
  void resample(u8** pcmData, u32& pcmDataSize, i32 inSampleRate, i32 outSampleRate);
}

#endif // PCM_H
