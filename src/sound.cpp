#include "sound.h"

#include "data.h"
#include "global.h"
#include "pcm.h"
#include "plugin.h"
#include "settings.h"

#include "chromagram.h"
#include "chordDetector.h"

#include <SDL2/SDL.h>

#include <vector>
#include <stdio.h>

#include <condition_variable>
#include <mutex>

static SDL_AudioDeviceID devid_in = 0;
static SDL_AudioSpec want_in;
union Buffer
{
  u8 sdl[Const::audioMaximumPossibleBufferSize * sizeof(f32) * 2]; // SDL format: L0, R0, L1, R1, ...
#ifdef SUPPORT_PLUGIN
  struct
  {
    u8 left[Const::audioMaximumPossibleBufferSize * sizeof(f32)]; // VST and VST3 format: L0, L1, ...
    u8 right[Const::audioMaximumPossibleBufferSize * sizeof(f32)];  // VST and VST3 format: R0, R1, ...
  } plugin;
#endif // SUPPORT_PLUGIN
};
static Buffer buffer0;
#ifdef SUPPORT_PLUGIN
static f32* buffer0Vst[2] = { (f32*)buffer0.plugin.left, (f32*)buffer0.plugin.right };
static Buffer buffer1;
static f32* buffer1Vst[2] = { (f32*)buffer1.plugin.left, (f32*)buffer1.plugin.right };
#endif // SUPPORT_PLUGIN
static u8 buffer_mixer[Const::audioMaximumPossibleBufferSize * sizeof(f32) * 2];
std::vector<double> frame(Global::settings.audioBufferSize);

static SDL_AudioDeviceID devid_out;
static SDL_AudioSpec want_out;

static std::condition_variable cv;
static std::mutex mutex;
static bool recordingFirst = true;

static Chromagram chromagram(Global::settings.audioBufferSize, Global::settings.audioSampleRate);
static ChordDetector chordDetector;

#ifndef __EMSCRIPTEN__
static void audioRecordingCallback(void* userdata, u8* stream, int len)
{
  ASSERT(len <= sizeof(buffer0.sdl));

  ++Global::debugAudioCallbackRecording;

  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, [] { return recordingFirst ? true : false; });

  const i32 channelOffset = Global::settings.audioChannelInstrument[0] == 0 ? 0 : 4;
  switch (Global::settings.audioSignalChain)
  {
  case SignalChain::soundBank:
    for (i32 i = 0; i < len; i += 8)
    {
      buffer0.sdl[i] = stream[i + channelOffset];
      buffer0.sdl[i + 1] = stream[i + channelOffset + 1];
      buffer0.sdl[i + 2] = stream[i + channelOffset + 2];
      buffer0.sdl[i + 3] = stream[i + channelOffset + 3];

      // Right Output Channel
      buffer0.sdl[i + 4] = stream[i + channelOffset];
      buffer0.sdl[i + 5] = stream[i + channelOffset + 1];
      buffer0.sdl[i + 6] = stream[i + channelOffset + 2];
      buffer0.sdl[i + 7] = stream[i + channelOffset + 3];
    }
    break;
  case SignalChain::plugin:
    for (i32 i = 0; i < len; i += 8)
    {
      buffer0.plugin.left[i / 2] = stream[i + channelOffset];
      buffer0.plugin.left[i / 2 + 1] = stream[i + channelOffset + 1];
      buffer0.plugin.left[i / 2 + 2] = stream[i + channelOffset + 2];
      buffer0.plugin.left[i / 2 + 3] = stream[i + channelOffset + 3];

      buffer0.plugin.right[i / 2] = stream[i + channelOffset];
      buffer0.plugin.right[i / 2 + 1] = stream[i + channelOffset + 1];
      buffer0.plugin.right[i / 2 + 2] = stream[i + channelOffset + 2];
      buffer0.plugin.right[i / 2 + 3] = stream[i + channelOffset + 3];
    }
    break;
  default:
    assert(false);
  }

  f32 instrumentVolume = 0.0f;
  for (i32 i = 0; i < len / 8; ++i)
  {
    const f32 value = reinterpret_cast<f32*>(stream)[i * 2 + 1];
    frame[i++] = value;
    instrumentVolume = max_(instrumentVolume, abs(value));
  }
  Global::instrumentVolume = instrumentVolume;

  chromagram.processAudioFrame(frame);

  if (chromagram.isReady())
  {
    const std::vector<double> chroma = chromagram.getChromagram();
    chordDetector.detectChord(chroma);

    Global::chordDetectorRootNote = Chords::Note((chordDetector.rootNote + 3) % 12);
    Global::chordDetectorQuality = Chords::Quality(chordDetector.quality);
    Global::chordDetectorIntervals = chordDetector.intervals;
  }

  recordingFirst = !recordingFirst;
  cv.notify_one();
}
#endif // __EMSCRIPTEN__

static void audioPlaybackCallback(void* userdata, u8* stream, i32 len)
{
  ASSERT(len <= sizeof(buffer0.sdl));

  ++Global::debugAudioCallbackPlayback;

#ifndef __EMSCRIPTEN__
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, [] { return !recordingFirst ? true : false; });
#endif // __EMSCRIPTEN__

  SDL_memset(stream, 0, len);
  switch (Global::settings.audioSignalChain)
  {
  case SignalChain::soundBank:
    SDL_MixAudioFormat(stream, buffer0.sdl, AUDIO_F32LSB, len, Global::settings.mixerGuitar1Volume);
    break;
  case SignalChain::plugin:
  {
    bool srcBuffer = 0;

    for (i32 i = 0; i < NUM(Global::effectChain); ++i)
    {
      if (Global::effectChain[i] < 0)
        continue;

      i32 instance = 0;
      for (i32 j = 0; j < i; ++j)
        if (Global::effectChain[i] == Global::effectChain[j])
          ++instance;

      switch (srcBuffer)
      {
      case 0:
        Plugin::processBlock(Global::effectChain[i], instance, buffer0Vst, buffer1Vst, (len / sizeof(f32)) / 2);
        break;
      case 1:
        Plugin::processBlock(Global::effectChain[i], instance, buffer1Vst, buffer0Vst, (len / sizeof(f32)) / 2);
        break;
      default:
        assert(false);
      }
      srcBuffer = (srcBuffer + 1) % 2;
    }

    switch (srcBuffer) // convert to sdl format and mix
    {
    case 0:
      for (i32 i = 0; i < len; i += 8)
      {
        buffer1.sdl[i] = buffer0.plugin.left[i / 2];
        buffer1.sdl[i + 1] = buffer0.plugin.left[i / 2 + 1];
        buffer1.sdl[i + 2] = buffer0.plugin.left[i / 2 + 2];
        buffer1.sdl[i + 3] = buffer0.plugin.left[i / 2 + 3];

        buffer1.sdl[i + 4] = buffer0.plugin.right[i / 2];
        buffer1.sdl[i + 5] = buffer0.plugin.right[i / 2 + 1];
        buffer1.sdl[i + 6] = buffer0.plugin.right[i / 2 + 2];
        buffer1.sdl[i + 7] = buffer0.plugin.right[i / 2 + 3];
      }
      SDL_MixAudioFormat(stream, buffer1.sdl, AUDIO_F32LSB, len, Global::settings.mixerGuitar1Volume);
      break;
    case 1:
      for (i32 i = 0; i < len; i += 8)
      {
        buffer0.sdl[i] = buffer1.plugin.left[i / 2];
        buffer0.sdl[i + 1] = buffer1.plugin.left[i / 2 + 1];
        buffer0.sdl[i + 2] = buffer1.plugin.left[i / 2 + 2];
        buffer0.sdl[i + 3] = buffer1.plugin.left[i / 2 + 3];

        buffer0.sdl[i + 4] = buffer1.plugin.right[i / 2];
        buffer0.sdl[i + 5] = buffer1.plugin.right[i / 2 + 1];
        buffer0.sdl[i + 6] = buffer1.plugin.right[i / 2 + 2];
        buffer0.sdl[i + 7] = buffer1.plugin.right[i / 2 + 3];
      }
      SDL_MixAudioFormat(stream, buffer0.sdl, AUDIO_F32LSB, len, Global::settings.mixerGuitar1Volume);
      break;
    default:
      assert(false);
    }
  }
  break;
  default:
    assert(false);
    break;
  }

  if (Global::musicBufferPosition != nullptr)
  {
    const i64 remainingLength = &Global::musicBuffer[Global::musicBufferLength] - Global::musicBufferPosition;
    if (remainingLength > 0)
    {
      len = i32(len > remainingLength ? remainingLength : len);
      SDL_MixAudioFormat(stream, Global::musicBufferPosition, AUDIO_F32LSB, len, Global::settings.mixerMusicVolume);

      Global::musicBufferPosition += len;
    }
    else
    {
      Global::musicBufferPosition = nullptr;
    }
  }

#ifndef __EMSCRIPTEN__
  recordingFirst = !recordingFirst;
  cv.notify_one();
#endif // __EMSCRIPTEN__
}

void Sound::init()
{
#ifndef __EMSCRIPTEN__
  { // Input
    SDL_memset(&want_in, 0, sizeof(want_in));

    want_in.freq = Global::settings.audioSampleRate;
    want_in.format = AUDIO_F32LSB;
    want_in.channels = 2;
    want_in.samples = Global::settings.audioBufferSize;
    want_in.callback = audioRecordingCallback;
    want_in.userdata = nullptr;

    devid_in = SDL_OpenAudioDevice(NULL, SDL_TRUE, &want_in, nullptr, 0);
    ASSERT(devid_in != 0);
  }
#endif // #ifndef __EMSCRIPTEN__

  { // Output
    SDL_AudioSpec have;
    SDL_memset(&want_out, 0, sizeof(want_out));

    want_out.freq = Global::settings.audioSampleRate;
    want_out.format = AUDIO_F32LSB;
    want_out.channels = 2;
    want_out.samples = Global::settings.audioBufferSize;
    want_out.callback = audioPlaybackCallback;
    want_out.userdata = nullptr;

    devid_out = SDL_OpenAudioDevice(NULL, 0, &(want_out), &have, 0);
    ASSERT(devid_out != 0);
  }
#ifndef __EMSCRIPTEN__
  SDL_PauseAudioDevice(devid_in, false);
#endif // #ifndef __EMSCRIPTEN__
  SDL_PauseAudioDevice(devid_out, false);
}
