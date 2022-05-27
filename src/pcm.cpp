#include "pcm.h"

#include "ogg.h"

#include <SDL2/SDL.h>

i32 Pcm::decodeOgg(const u8* oggData, u32 oggDataSize, u8** pcmData, u32& pcmDataSize)
{
  pcmDataSize = 0;
  *pcmData = nullptr;

  Ogg::vorbis* vorbis = Ogg::open(oggData, oggDataSize);

  Ogg::Info oggInfo = Ogg::getInfo(vorbis);
  assert(oggInfo.channels == 2);
  const i32 sampleRate = oggInfo.sample_rate;

  //const u64 chunkAllocSize = 65536;
  //for (;;)
  //{
  //  audio->lengthTrue += chunkAllocSize;
  //  audio->bufferTrue = (u8*)realloc(audio->bufferTrue, audio->lengthTrue * sizeof(f32));
  //  const i32 samples = Ogg::getSamplesInterleaved(vorbis, info.channels, (f32*)&audio->bufferTrue[(audio->lengthTrue - chunkAllocSize) * sizeof(f32)], chunkAllocSize);
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
    const i32 samples = Ogg::getSamplesInterleaved(vorbis, oggInfo.channels, buffer, NUM(buffer));
    if (samples > 0)
    {
      const i64 dataSize = samples * oggInfo.channels * sizeof(f32);
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

  Ogg::close(vorbis);

  return sampleRate;
}

// TODO: replace with: https://wiki.libsdl.org/Tutorials-AudioStream
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
