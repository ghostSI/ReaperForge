#include "sound.h"
#include "data.h"
#include "global.h"
#include "oggvorbis.h"
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
  Effect = 10000, // play once
  Music = 20000, // play until another on should be played
  Ogg = 30000,
};

struct Audio
{
  i32 soundId = 0;

  u32 length = 0;
  u32 lengthTrue = 0;
  u8* bufferTrue = nullptr;
  u8* buffer = nullptr;
  stb_vorbis* vorbis = nullptr;
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
    if (audios[i].vorbis == nullptr && audios[i].bufferTrue == nullptr)
      return &audios[i];

  return nullptr;
}

static SoundType getSoundType(Audio* audio)
{
  if (audio->soundId >= to_underlying(SoundType::Ogg))
    return SoundType::Ogg;
  if (audio->soundId >= to_underlying(SoundType::Music))
    return SoundType::Music;
  return SoundType::Effect;
}

static void addMusic(Audio* new1)
{
  bool musicFound = false; // only one music can be active at a time
  Audio* headNext = newAudio();

  /* Phase out any current music */
  if (getSoundType(headNext) == SoundType::Music && headNext->fade == 0)
  {
    if (musicFound)
    {
      headNext->length = 0;
      headNext->volume = 0;
    }

    headNext->fade = 1;
  }
  /* Set recordingFirst to remove any queued up music in favour of new music */
  else if (getSoundType(headNext) == SoundType::Music && headNext->fade == 1)
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

  if (soundType == SoundType::Music)
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
      if (audio->fade == 1 && getSoundType(audio) == SoundType::Music)
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

      if (music && getSoundType(audio) == SoundType::Music && audio->fade == 0)
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
    else if (getSoundType(audio) == SoundType::Music && audio->fade == 0)
    {
      audio->buffer = audio->bufferTrue;
      audio->length = audio->lengthTrue;
    }
    else if (i32 samples; getSoundType(audio) == SoundType::Ogg && (samples = stb_vorbis_get_samples_float_interleaved(audio->vorbis, 2, (f32*)buffer_mixer, len / sizeof(f32))) > 0)
    {
      SDL_MixAudio(stream, buffer_mixer, samples * 8, Global::settings.mixerMusicVolume);
    }
    else
    {
      if (audio->soundId != 0)
      {
        // remove audio
        if (audio->free == 1)
        {
          if (audio->vorbis)
            stb_vorbis_close(audio->vorbis);
          if (audio->bufferTrue)
            SDL_FreeWAV(audio->bufferTrue);
        }
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
    audio.soundId = 10000;
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
    audio.soundId = 10001;
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

void Sound::playOgg()
{
  SDL_PauseAudioDevice(devid_out, 1);

  Audio* audio = newAudio();
  if (audio == nullptr)
  {
    SDL_PauseAudioDevice(devid_out, 0);
    return;
  }

  audio->vorbis = stb_vorbis_open_memory(Global::ogg.data(), Global::ogg.size(), nullptr, nullptr);
  audio->soundId = 30000;
  audio->free = 1;
  Global::oggStartTime = Global::time;


  stb_vorbis_info info = stb_vorbis_get_info(audio->vorbis);

  audio->spec = SDL_AudioSpec();
  audio->spec.freq = info.sample_rate;
  audio->spec.format = AUDIO_F32LSB;
  audio->spec.channels = info.channels;
  audio->spec.samples = Global::settings.audioBufferSize;
  audio->spec.callback = audioPlaybackCallback;
  audio->spec.userdata = nullptr;


  SDL_OpenAudio(&audio->spec, NULL);

  SDL_PauseAudioDevice(devid_out, 0);
}

void Sound::play(Sound::Effect effect, i32 volume)
{
  playAudio(&soundPool[to_underlying(effect)], SoundType::Effect, volume);
}

void Sound::setPauseAudio(bool pauseAudio)
{
  SDL_PauseAudioDevice(devid_out, pauseAudio);
}
