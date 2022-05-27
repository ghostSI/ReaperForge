#ifndef OGG_H
#define OGG_H

#include "typedefs.h"

namespace Ogg
{
  struct Info
  {
    u32 sample_rate;
    i32 channels;
    u32 setup_memory_required;
    u32 setup_temp_memory_required;
    u32 temp_memory_required;
    i32 max_frame_size;
  };
  struct vorbis;

  vorbis* open(const u8* data, i32 len);
  void close(vorbis* f);

  Info getInfo(vorbis* f);

  int getSamplesInterleaved(vorbis* f, i32 channels, f32* buffer, i32 num_floats);
}

#endif // OGG_H