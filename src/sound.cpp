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

enum struct SoundType : i32
{
  empty,
  Wav,
  Ogg
};

struct Audio
{
  SoundType soundType = SoundType::empty;

  u32 length = 0;
  u32 lengthTrue = 0;
  u8* bufferTrue = nullptr;
  u8* buffer = nullptr;
  u8 fade = 0;
  u8 free = 0;
  u8 volume = 0;

  SDL_AudioSpec spec;
};

static Audio audios[Const::soundMaxCount];

static std::vector<Audio> soundPool;
static std::vector<Audio> musicPool;


static Audio* newAudio()
{
  for (i32 i = 0; i < Const::soundMaxCount; ++i)
    if (audios[i].bufferTrue == nullptr)
      return &audios[i];

  return nullptr;
}

static void addMusic(Audio* new1)
{
  bool musicFound = false; // only one music can be active at a time
  Audio* headNext = newAudio();

  /* Phase out any current music */
  if (headNext->soundType == SoundType::Ogg && headNext->fade == 0)
  {
    if (musicFound)
    {
      headNext->length = 0;
      headNext->volume = 0;
    }

    headNext->fade = 1;
  }
  /* Set recordingFirst to remove any queued up music in favour of new music */
  else if (headNext->soundType == SoundType::Ogg && headNext->fade == 1)
  {
    musicFound = true;
  }
}

static void playAudio(Audio* audio, SoundType soundType, i32 volume)
{
  ASSERT(volume >= 0 && volume <= 128);

  Audio* new1 = newAudio();

  *new1 = *audio;

  new1->volume = volume;
  new1->free = 0;

  /* Lock callback function */
  SDL_LockAudioDevice(devid_out);

  if (soundType == SoundType::Ogg)
  {
    addMusic(new1);
  }
  else
  {
    //addAudio(new1);
  }

  SDL_UnlockAudioDevice(devid_out);
}

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

//param userdata      Poi32s to linked list of sounds to play, first being a placeholder
//param stream        Stream to mix sound into
//param len           Length of sound to play
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

  for (i32 i = 0; i < Const::soundMaxCount; ++i)
  {
    Audio* audio = &audios[i];
    i32 tempLength;

    if (audio->length > 0)
    {
      bool music = false;
      if (audio->fade == 1 && audio->soundType == SoundType::Ogg)
      {
        music = true;

        if (audio->volume > 0)
        {
          audio->volume--;
        }
        else
        {
          audio->length = 0;
        }
      }

      if (music && audio->soundType == SoundType::Ogg && audio->fade == 0)
      {
        tempLength = 0;
      }
      else
      {
        tempLength = ((u32)len > audio->length) ? audio->length : (u32)len;
      }

      SDL_MixAudio(stream, audio->buffer, tempLength, audio->volume);

      audio->buffer += tempLength;
      audio->length -= tempLength;
    }
    else if (audio->soundType == SoundType::Ogg && audio->fade == 0)
    {
      audio->buffer = audio->bufferTrue;
      audio->length = audio->lengthTrue;
    }
    else
    {
      // remove audio
      if (audio->free == 1)
      {
        assert(audio->bufferTrue != nullptr);
        if (audio->soundType == SoundType::Ogg)
          free(audio->bufferTrue);
        else
          SDL_FreeWAV(audio->bufferTrue);
        *audio = Audio();
      }
    }
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
    SDL_memset(&want_out, 0, sizeof(want_out));

    want_out.freq = Global::settings.audioSampleRate;
    want_out.format = AUDIO_F32LSB;
    want_out.channels = 2;
    want_out.samples = Global::settings.audioBufferSize;
    want_out.callback = audioPlaybackCallback;
    want_out.userdata = nullptr;

    devid_out = SDL_OpenAudioDevice(NULL, 0, &(want_out), nullptr, 0);
    ASSERT(devid_out != 0);

    SDL_PauseAudioDevice(devid_out, 0);
  }

  // Effects
  {
    Audio audio;
    audio.soundType = SoundType::Wav;
    audio.fade = 0;
    audio.free = 1;
    audio.volume = 64;
    const SDL_AudioSpec* spec = SDL_LoadWAV_RW(SDL_RWFromMem((void*)Data::Sound::menuHoverWav, sizeof(Data::Sound::menuHoverWav)), 1, &audio.spec, &audio.bufferTrue, &audio.lengthTrue);
    ASSERT(spec != nullptr);
    audio.buffer = audio.bufferTrue;
    audio.length = audio.lengthTrue;
    audio.spec.callback = audioPlaybackCallback;
    audio.spec.userdata = nullptr;
    soundPool.push_back(audio);
  }
  {
    Audio audio;
    audio.soundType = SoundType::Wav;
    audio.fade = 0;
    audio.free = 1;
    audio.volume = 64;
    const SDL_AudioSpec* spec = SDL_LoadWAV_RW(SDL_RWFromMem((void*)Data::Sound::menuSelectWav, sizeof(Data::Sound::menuSelectWav)), 1, &audio.spec, &audio.bufferTrue, &audio.lengthTrue);
    ASSERT(spec != nullptr);
    audio.buffer = audio.bufferTrue;
    audio.length = audio.lengthTrue;
    audio.spec.callback = audioPlaybackCallback;
    audio.spec.userdata = nullptr;
    soundPool.push_back(audio);
  }
}

void Sound::fini()
{
  SDL_PauseAudioDevice(devid_out, 1);
}

void Sound::playOgg()
{
  SDL_PauseAudioDevice(devid_out, 1);

  Audio* audio = newAudio();
  if (audio == nullptr)
  {
    SDL_PauseAudioDevice(devid_out, 0);
    return;
  }

  // TODO: decoding and resampling takes a lot of time and memory. A good approch would be to decode and resample only small chunks of data.
  const i32 sampleRate = Pcm::decodeOgg(Global::ogg.data(), Global::ogg.size(), &audio->bufferTrue, audio->lengthTrue);
  Pcm::resample(&audio->bufferTrue, audio->lengthTrue, sampleRate, Global::settings.audioSampleRate);

  audio->soundType = SoundType::Ogg;
  audio->free = 1;
  audio->spec = SDL_AudioSpec();
  audio->spec.freq = Global::settings.audioSampleRate;
  audio->spec.format = AUDIO_F32LSB;
  audio->spec.channels = 2;
  audio->spec.samples = Global::settings.audioBufferSize;
  audio->spec.callback = audioPlaybackCallback;
  audio->spec.userdata = nullptr;
  audio->length = audio->lengthTrue;
  audio->buffer = audio->bufferTrue;
  audio->volume = 64;

  SDL_OpenAudio(&audio->spec, NULL);

  Global::oggStartTime = Global::time;
  SDL_PauseAudioDevice(devid_out, 0);
}

void stopOgg()
{
  SDL_PauseAudioDevice(devid_out, 1);

  Global::oggStartTime = 0.0f;

  for (i32 i = 0; i < Const::soundMaxCount; ++i)
    if (audios[i].bufferTrue != nullptr && audios[i].soundType == SoundType::Ogg)
      audios[i] = Audio();

  SDL_PauseAudioDevice(devid_out, 0);
}

void Sound::play(Sound::Effect effect, i32 volume)
{
  playAudio(&soundPool[to_underlying(effect)], SoundType::Wav, volume);
}

void Sound::setPauseAudio(bool pauseAudio)
{
  SDL_PauseAudioDevice(devid_out, pauseAudio);
}
