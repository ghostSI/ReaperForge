#include "sound.h"
#include "oggvorbis.h"
#include "global.h"
#include "settings.h"

#include "chromagram.h"
#include "chordDetector.h"

#include <SDL2/SDL.h>

#include <vector>
#include <stdio.h>

#include <mutex>

namespace Const
{
  // Max number of sounds that can be in the audio queue at anytime, stops too much mixing
  static constexpr u32 maxSoundEffects = 25;
}

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
  Audio* next = nullptr;
};

static Audio head;
static u32 maxSoundEffects;

static std::vector<Audio> soundPool;
static std::vector<Audio> musicPool;


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
  if (audio->soundId >= to_underlying(SoundType::Ogg))
    return SoundType::Ogg;
  if (audio->soundId >= to_underlying(SoundType::Music))
    return SoundType::Music;
  return SoundType::Effect;
}

static void addMusic(Audio* new1)
{
  bool musicFound = false; // only one music can be active at a time
  Audio* headNext = head.next;

  /* Find any existing musics, 0, 1 or 2 and fade them out */
  while (headNext != nullptr)
  {
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

    headNext = headNext->next;
  }

  addAudio(new1);
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
  SDL_LockAudioDevice(devid_out);

  if (soundType == SoundType::Music)
  {
    addMusic(new1);
  }
  else
  {
    addAudio(new1);
  }

  SDL_UnlockAudioDevice(devid_out);
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
static void audioRecordingCallback(void* userdata, u8* stream, int len)
{
  ASSERT(len <= sizeof(buffer_in));

  ++Global::debugAudioCallbackRecording;

  //std::unique_lock<std::mutex> lock(mutex);
  //cv.wait(lock, [] { return recordingFirst ? true : false; });


  // right channel to stereo
  for (i32 i = 0; i < len; i += 8)
  {
    // Left Output Channel
    buffer_in[i] = stream[i + 4];
    buffer_in[i + 1] = stream[i + 5];
    buffer_in[i + 2] = stream[i + 6];
    buffer_in[i + 3] = stream[i + 7];

    // Right Output Channel
    buffer_in[i + 4] = stream[i + 4];
    buffer_in[i + 5] = stream[i + 5];
    buffer_in[i + 6] = stream[i + 6];
    buffer_in[i + 7] = stream[i + 7];
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

    Global::chordDetectorRootNote = Chords::Note((chordDetector.rootNote + 3)  % 12);
    Global::chordDetectorQuality = Chords::Quality(chordDetector.quality);
    Global::chordDetectorIntervals = chordDetector.intervals;
  }

  recordingFirst = !recordingFirst;
  //cv.notify_one();
}

//param userdata      Poi32s to linked list of sounds to play, first being a placeholder
//param stream        Stream to mix sound into
//param len           Length of sound to play
static void audioPlaybackCallback(void* userdata, u8* stream, i32 len)
{
  ASSERT(len <= sizeof(buffer_in));

  ++Global::debugAudioCallbackPlayback;

  //std::unique_lock<std::mutex> lock(mutex);
  //cv.wait(lock, [] { return !recordingFirst ? true : false; });

  SDL_memset(stream, 0, len);

  SDL_MixAudioFormat(stream, buffer_in, AUDIO_F32LSB, len, Global::settings.mixerGuitar1Volume);

  Audio* audio = reinterpret_cast<Audio*>(userdata);
  Audio* previous = audio;
  i32 tempLength;
  u8 music = 0;

  /* First one is place holder */
  audio = audio->next;

  while (audio != nullptr)
  {
    if (audio->length > 0)
    {
      if (audio->fade == 1 && getSoundType(audio) == SoundType::Music)
      {
        music = 1;

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

      SDL_MixAudioFormat(stream, audio->buffer, AUDIO_F32LSB, tempLength, audio->volume);

      audio->buffer += tempLength;
      audio->length -= tempLength;

      previous = audio;
      audio = audio->next;
    }
    else if (getSoundType(audio) == SoundType::Music && audio->fade == 0)
    {
      audio->buffer = audio->bufferTrue;
      audio->length = audio->lengthTrue;

      previous = audio;
      audio = audio->next;
    }
    else if (getSoundType(audio) == SoundType::Ogg)
    {
      stb_vorbis_get_samples_float_interleaved(audio->vorbis, 2, (f32*)buffer_mixer, len / sizeof(f32));
      SDL_MixAudio(stream, buffer_mixer, len, Global::settings.mixerMusicVolume);

      previous = audio;
      audio = audio->next;
    }
    else
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

  recordingFirst = !recordingFirst;
  //cv.notify_one();
}

static void initAudio()
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
    want_out.userdata = &head;

    devid_out = SDL_OpenAudioDevice(NULL, 0, &(want_out), nullptr, 0);
    ASSERT(devid_out != 0);

    SDL_PauseAudioDevice(devid_out, 0);
  }
}

static void initSound(Sound::Effect soundType2, const char* filepath)
{
  soundPool.push_back(loadAudioFile(filepath, to_underlying(SoundType::Effect) + to_underlying(soundType2), 128));
}

static void initSound(Sound::Music soundType2, const char* filepath)
{
  musicPool.push_back(loadAudioFile(filepath, to_underlying(SoundType::Music) + to_underlying(soundType2), 128));
}


void Sound::init()
{
  initAudio();

  // Effects
  //initSound(Sound::Effect::menuHover, "res/menuHover.wav");
  //initSound(Sound::Effect::menuSelect, "res/menuSelect.wav");
}

void Sound::tick()
{
}

void Sound::playOgg()
{
  Audio* audio = new Audio;

  audio->vorbis = stb_vorbis_open_memory(Global::ogg.data(), Global::ogg.size(), nullptr, nullptr);
  audio->soundId = 30000;
  Global::oggStartTime = Global::time;


  stb_vorbis_info info = stb_vorbis_get_info(audio->vorbis);

  SDL_AudioSpec spec;
  spec.freq = info.sample_rate;
  spec.format = AUDIO_F32LSB;
  spec.channels = info.channels;
  spec.samples = Global::settings.audioBufferSize;
  spec.callback = audioPlaybackCallback;
  spec.userdata = audio;

  SDL_OpenAudio(&spec, NULL);

  addAudio(audio);

  SDL_PauseAudio(0);
}

void Sound::play(Sound::Effect effect, i32 volume)
{
  playAudio(&soundPool[to_underlying(effect)], SoundType::Effect, volume);
}

void Sound::play(Sound::Music music, i32 volume)
{
  playAudio(&musicPool[to_underlying(music)], SoundType::Music, volume);
}

void Sound::setPauseAudio(bool pauseAudio)
{
  SDL_PauseAudioDevice(devid_out, pauseAudio);
}
