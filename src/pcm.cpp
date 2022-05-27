#include "pcm.h"

#include "oggvorbis.h"

#include <SDL2/SDL.h>

i32 Pcm::decodeOgg(u8* oggData, u32 oggDataSize, u8** pcmData, u32& pcmDataSize)
{
  pcmDataSize = 0;
  *pcmData = nullptr;

  stb_vorbis* vorbis = stb_vorbis_open_memory(oggData, oggDataSize, nullptr, nullptr);

  stb_vorbis_info info = stb_vorbis_get_info(vorbis);
  assert(info.channels == 2);
  const i32 sampleRate = info.sample_rate;

  //const u64 chunkAllocSize = 65536;
  //for (;;)
  //{
  //  audio->lengthTrue += chunkAllocSize;
  //  audio->bufferTrue = (u8*)realloc(audio->bufferTrue, audio->lengthTrue * sizeof(f32));
  //  const i32 samples = stb_vorbis_get_samples_float_interleaved(vorbis, info.channels, (f32*)&audio->bufferTrue[(audio->lengthTrue - chunkAllocSize) * sizeof(f32)], chunkAllocSize);
  //  if (samples == 0)
  //    break;
  //  if (samples * info.channels != chunkAllocSize)
  //  {
  //    audio->lengthTrue = audio->lengthTrue - chunkAllocSize + samples * info.channels;
  //    audio->bufferTrue = (u8*)realloc(audio->bufferTrue, audio->lengthTrue * sizeof(f32));
  //    break;
  //  }
  //}

  for (;;)
  {
    f32 buffer[131072];
    const i32 samples = stb_vorbis_get_samples_float_interleaved(vorbis, info.channels, buffer, NUM(buffer));
    if (samples > 0)
    {
      const i64 dataSize = samples * info.channels * sizeof(f32);
      pcmDataSize += dataSize;
      *pcmData = (u8*)realloc(*pcmData, pcmDataSize);
      assert(pcmData != nullptr);
      memcpy(&(*pcmData)[pcmDataSize - dataSize], buffer, dataSize);
    }
    else
    {
      break;
    }
  }

  stb_vorbis_close(vorbis);

  return sampleRate;
}

void Pcm::resample(u8** pcmData, u32& pcmDataSize, i32 inSampleRate, i32 outSampleRate)
{
  SDL_AudioCVT cvt;
  if (SDL_BuildAudioCVT(&cvt, AUDIO_F32, 2, inSampleRate, AUDIO_F32, 2, outSampleRate) == 1)
  {
    cvt.buf = (u8*)malloc(pcmDataSize * cvt.len_mult);
    cvt.len = pcmDataSize;
    memcpy(cvt.buf, *pcmData, pcmDataSize);


    i32 result = SDL_ConvertAudio(&cvt);
    assert(0 == result);

    *pcmData = (u8*)realloc(*pcmData, cvt.len * cvt.len_mult);
    memcpy(*pcmData, cvt.buf, cvt.len * cvt.len_mult);
    pcmDataSize = cvt.len * cvt.len_ratio;
  }
}
