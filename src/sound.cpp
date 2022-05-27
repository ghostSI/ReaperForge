#include "sound.h"

#include "data.h"
#include "global.h"
#include "pcm.h"
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
static u8 buffer_in[Const::audioMaximumPossibleBufferSize * sizeof(f32) * 2];
static u8 buffer_mixer[Const::audioMaximumPossibleBufferSize * sizeof(f32) * 2];
std::vector<double> frame(Global::settings.audioBufferSize);

static SDL_AudioDeviceID devid_out;
static SDL_AudioSpec want_out;

static std::condition_variable cv;
static std::mutex mutex;
bool recordingFirst = true;

static Chromagram chromagram(Global::settings.audioBufferSize, Global::settings.audioSampleRate);
static ChordDetector chordDetector;

static void audioRecordingCallback(void* userdata, u8* stream, int len)
{
  ASSERT(len <= sizeof(buffer_in));

  if (Global::appQuit)
    return;

  ++Global::debugAudioCallbackRecording;

  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, [] { return recordingFirst ? true : false; });

  const i32 channelOffset = Global::settings.audioChannelInstrument[0] == 0 ? 0 : 4;

  // channel instrument0 to stereo
  for (i32 i = 0; i < len; i += 8)
  {
    // Left Output Channel
    buffer_in[i] = stream[i + channelOffset];
    buffer_in[i + 1] = stream[i + channelOffset + 1];
    buffer_in[i + 2] = stream[i + channelOffset + 2];
    buffer_in[i + 3] = stream[i + channelOffset + 3];

    // Right Output Channel
    buffer_in[i + 4] = stream[i + channelOffset];
    buffer_in[i + 5] = stream[i + channelOffset + 1];
    buffer_in[i + 6] = stream[i + channelOffset + 2];
    buffer_in[i + 7] = stream[i + channelOffset + 3];
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

static void audioPlaybackCallback(void* userdata, u8* stream, i32 len)
{
  ASSERT(len <= sizeof(buffer_in));

  if (Global::appQuit)
    return;

  ++Global::debugAudioCallbackPlayback;

  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, [] { return !recordingFirst ? true : false; });

  SDL_memset(stream, 0, len);
  SDL_MixAudioFormat(stream, buffer_in, AUDIO_F32LSB, len, Global::settings.mixerGuitar1Volume);

  if (Global::audioMusicRemainingLength > 0)
  {
    len = (len > Global::audioMusicRemainingLength ? Global::audioMusicRemainingLength : len);
    SDL_MixAudioFormat(stream, Global::audioMusicBufferPosition, AUDIO_F32LSB, len, Global::settings.mixerMusicVolume);

    Global::audioMusicBufferPosition += len;
    Global::audioMusicRemainingLength -= len;
  }

  recordingFirst = !recordingFirst;
  cv.notify_one();
}

void Sound::init()
{
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

    SDL_PauseAudioDevice(devid_in, 0);
  }

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

    SDL_PauseAudioDevice(devid_out, 0);
  }
}

void Sound::fini()
{
  pauseAudioDevice(true);
}

void Sound::pauseAudioDevice(bool pause_on)
{
  SDL_PauseAudioDevice(devid_in, pause_on);
  SDL_PauseAudioDevice(devid_out, pause_on);
}
