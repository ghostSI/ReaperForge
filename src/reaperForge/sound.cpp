#include "sound.h"
#include "log.h"

#include <SDL2/SDL.h>

#include <vector>
#include <stdio.h>

namespace Const
{
// Max number of sounds that can be in the audio queue at anytime, stops too much mixing
static constexpr u32 maxSoundEffects = 25;
}

struct PrivateAudioDevice
{
  SDL_AudioDeviceID device;
  SDL_AudioSpec want;
  u8 audioEnabled;
};

static PrivateAudioDevice gDevice;

enum struct SoundType : i32
{
  Effect = 10000, // play once
};

struct Audio
{
  i32 soundId = 0;

  u32 length = 0;
  u32 lengthTrue = 0;
  u8* bufferTrue = nullptr;
  u8* buffer = nullptr;
  u8 fade = 0;
  u8 free = 0;
  u8 volume = 0;

  SDL_AudioSpec spec;
  Audio* next = nullptr;
};

static Audio head;
static u32 maxSoundEffects;

static std::vector<Audio> soundPool;
static std::vector<Audio> musicPool;
static std::vector<Audio> ambientPool;
static std::vector<Audio> blockTypePool;


static void addAudio(Audio* new1, Audio* root = nullptr)
{
  Audio* headNext = root != nullptr ? root : &head;

  while (headNext->next != nullptr)
  {
    headNext = headNext->next;
  }

  headNext->next = new1;
}

static SoundType getSoundType(Audio* audio)
{
  return SoundType::Effect;
}

static Audio loadAudioFile(const char* filename, u32 soundId, i32 volume)
{
  Audio audio;

  //ASSERT(filename != nullptr);

  audio.soundId = soundId;

  audio.fade = 0;
  audio.free = 1;
  audio.volume = volume;

  const SDL_AudioSpec* spec = SDL_LoadWAV(filename, &audio.spec, &audio.bufferTrue, &audio.lengthTrue);
  ASSERT(spec != nullptr);

  audio.buffer = audio.bufferTrue;
  audio.length = audio.lengthTrue;
  audio.spec.callback = nullptr;
  audio.spec.userdata = nullptr;

  return audio;
}

static void playAudio(Audio* audio, SoundType soundType, i32 volume)
{
  ASSERT(volume >= 0 && volume <= 128);

  if (!gDevice.audioEnabled)
    return;

  /* If sound, check if under max number of sounds allowed, else don't play */
  if (soundType == SoundType::Effect)
  {
    if (maxSoundEffects >= Const::maxSoundEffects)
    {
      return;
    }
    else
    {
      maxSoundEffects++;
    }
  }

  Audio* new1 = new Audio;

  memcpy(new1, audio, sizeof(Audio));

  new1->volume = volume;
  new1->free = 0;

  /* Lock callback function */
  SDL_LockAudioDevice(gDevice.device);

  addAudio(new1);

  SDL_UnlockAudioDevice(gDevice.device);
}

/*
* Frees as many chained Audios as given
*
* @param audio     Chain of sounds to free
*
*/
static void freeAudio(Audio* audio)
{
  Audio* temp;

  while (audio != nullptr)
  {
    if (audio->free == 1)
    {
      SDL_FreeWAV(audio->bufferTrue);
    }

    temp = audio;
    audio = audio->next;

    delete temp;
  }
}

//param userdata      Poi32s to linked list of sounds to play, first being a placeholder
//param stream        Stream to mix sound into
//param len           Length of sound to play
static void audioCallback(void* userdata, u8* stream, i32 len)
{
  Audio* audio = reinterpret_cast<Audio*>(userdata);
  Audio* previous = audio;
  i32 tempLength;
  u8 music = 0;

  /* Silence the main buffer */
  SDL_memset(stream, 0, len);

  /* First one is place holder */
  audio = audio->next;

  while (audio != nullptr)
  {
    if (audio->length > 0)
    {
      
      tempLength = ((u32)len > audio->length) ? audio->length : (u32)len;

      SDL_MixAudioFormat(stream, audio->buffer, AUDIO_S16LSB, tempLength, audio->volume);

      audio->buffer += tempLength;
      audio->length -= tempLength;

      previous = audio;
      audio = audio->next;
    }
    
    {
      { // remove audio
        previous->next = audio->next;
        if (getSoundType(audio) == SoundType::Effect)
        {
          maxSoundEffects--;
        }
        audio->next = nullptr;
        freeAudio(audio);
        audio = previous->next;
      }
    }
  }
}

static void initAudio()
{
  gDevice.audioEnabled = 0;

  SDL_memset(&(gDevice.want), 0, sizeof(gDevice.want));

  gDevice.want.freq = 48000;
  gDevice.want.format = AUDIO_S16LSB;
  gDevice.want.channels = 2;
  gDevice.want.samples = 4096;
  gDevice.want.callback = audioCallback;
  gDevice.want.userdata = &head;

  /* want.userdata = new; */
  if ((gDevice.device = SDL_OpenAudioDevice(NULL, 0, &(gDevice.want), nullptr, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE)) == 0)
  {
    fprintf(stderr, "[%s: %d]Warning: failed to open audio device: %s\n", __FILE__, __LINE__, SDL_GetError());
  }
  else
  {
    /* Set audio device enabled global flag */
    gDevice.audioEnabled = 1;

    /* Unpause active audio stream */
    if (gDevice.audioEnabled)
    {
      SDL_PauseAudioDevice(gDevice.device, 0);
    }
  }
}

/*static u32 soundType2soundId(SoundType soundType, Sound::Effect soundType2)
{
  return to_underlying(soundType) + to_underlying(soundType2);
}*/

static void initSound(Sound::Effect soundType2, const char* filepath)
{
  soundPool.push_back(loadAudioFile(filepath, to_underlying(SoundType::Effect) + to_underlying(soundType2), 128));
}

void Sound::init()
{
  initAudio();

  // Effects
  initSound(Sound::Effect::menuHover, "res/menuHover.wav");
  initSound(Sound::Effect::menuSelect, "res/menuSelect.wav");
}

void Sound::play(Sound::Effect effect, i32 volume)
{
  playAudio(&soundPool[to_underlying(effect)], SoundType::Effect, volume);
}

void Sound::setPauseAudio(bool pauseAudio)
{
  if (gDevice.audioEnabled)
    SDL_PauseAudioDevice(gDevice.device, pauseAudio);
}
