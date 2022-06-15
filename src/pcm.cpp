#include "pcm.h"

#include "ogg.h"

#include <SDL2/SDL.h>

i32 Pcm::decodeOgg(const u8* oggData, u64 oggDataSize, u8** pcmData, u64& pcmDataSize)
{
  pcmDataSize = 0;
  *pcmData = nullptr;

  Ogg::vorbis* vorbis = Ogg::open(oggData, i32(oggDataSize));

  Ogg::Info oggInfo = Ogg::getInfo(vorbis);
  assert(oggInfo.channels == 2);

  const u64 bufferChunkSize = 1073741824;
  u64 accumBufferSize = 0;
  do
  {
    *pcmData = (u8*)realloc(*pcmData, accumBufferSize + bufferChunkSize);
    assert(*pcmData != nullptr);
    const i32 samples = Ogg::getSamplesInterleaved(vorbis, oggInfo.channels, (f32*)&((*pcmData)[accumBufferSize]), bufferChunkSize / sizeof(f32));
    pcmDataSize += samples * oggInfo.channels * sizeof(f32);
    accumBufferSize += bufferChunkSize;
  } while (accumBufferSize == pcmDataSize);

  Ogg::close(vorbis);

  return oggInfo.sample_rate;
}

// TODO: replace with: https://wiki.libsdl.org/Tutorials-AudioStream
void Pcm::resample(u8** pcmData, u64& pcmDataSize, i32 inSampleRate, i32 outSampleRate)
{
  SDL_AudioCVT cvt{};
  if (SDL_BuildAudioCVT(&cvt, AUDIO_F32, 2, inSampleRate, AUDIO_F32, 2, outSampleRate) == 1)
  {
    cvt.buf = (u8*)malloc(pcmDataSize * cvt.len_mult);
    cvt.len = i32(pcmDataSize);
    memcpy(cvt.buf, *pcmData, pcmDataSize);

    i32 result = SDL_ConvertAudio(&cvt);
    assert(0 == result);

    *pcmData = (u8*)realloc(*pcmData, cvt.len * cvt.len_mult);
    memcpy(*pcmData, cvt.buf, cvt.len * cvt.len_mult);
    pcmDataSize = u64(cvt.len * cvt.len_ratio);
  }
}
