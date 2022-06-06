#include "player.h"

#include "global.h"
#include "helper.h"
#include "psarc.h"
#include "song.h"
#include "player.h"
#include "wem.h"
#include "pcm.h"
#include "sound.h"

static bool playNextTick = false;
static bool previewPlaying = false;

static u32 readWemFileIdFromBnkFile(const u8* data, u64 size)
{
  const u32 lenBKHD = u32_le(&data[4]);

  ASSERT(lenBKHD == 28);

  const u32 offset = lenBKHD + 12;

  const u32 lenDIDX = u32_le(&data[offset]);

  ASSERT(lenDIDX == 12);
  ASSERT(offset == 40);

  const u32 fileId = u32_le(&data[offset + 4]);

  return fileId;
}

static void loadAudio(const Psarc::Info& psarcInfo, bool preview)
{
  for (const Psarc::Info::TOCEntry& tocEntry : psarcInfo.tocEntries)
  {
    if ((preview && !tocEntry.name.ends_with("_preview.bnk")) || !tocEntry.name.ends_with(".bnk"))
      continue;

    const u32 wemFileId = readWemFileIdFromBnkFile(tocEntry.content.data(), tocEntry.content.size());

    char wemFileName[40];
    sprintf(wemFileName, "%u.wem", wemFileId);

    for (const Psarc::Info::TOCEntry& tocEntry : psarcInfo.tocEntries)
    {
      if (!tocEntry.name.ends_with(wemFileName))
        continue;
     
      const std::vector<u8> ogg = Wem::to_ogg(tocEntry.content.data(), tocEntry.length);
      i32 sampleRate = Pcm::decodeOgg(ogg.data(), ogg.size(), &Global::musicBuffer, Global::musicBufferLength);
      Pcm::resample(&Global::musicBuffer, Global::musicBufferLength, sampleRate, Global::settings.audioSampleRate);

      playNextTick = true;
      return;
    }
  }

  ASSERT(false);
}

static void playSongEmscripten()
{
  const std::vector<u8> psarcData = Psarc::readPsarcData(EMSC_PATH(psarc/test.psarc));

  const Psarc::Info psarcInfo = Psarc::parse(psarcData);
  Global::songInfos.push_back(Song::loadSongInfoManifestOnly(psarcInfo));
  Global::songSelected = 0;
  Song::loadSongInfoComplete(psarcInfo, Global::songInfos[Global::songSelected]);

  Global::songTrack = Song::loadTrack(psarcInfo, InstrumentFlags::LeadGuitar);
  Global::songVocals = Song::loadVocals(psarcInfo);

  Sound::pauseAudioDevice(true);
  loadAudio(psarcInfo, false);

  playNextTick = true;
}

static f32 quickRepeaterBeginTime = 0.0f;
static f32 quickRepeaterEndTime = 0.0f;
static u8* quickRepeaterMusicBufferPosition = nullptr;
static u32 quickRepeaterMusicRemainingLength = 0;

static void quickRepeater()
{
  if (Global::quickRepeater.pressed && !Global::quickRepeater.pressedLastFrame)
  {
    if (quickRepeaterBeginTime == 0.0f)
    {
      quickRepeaterBeginTime = Global::musicTimeElapsed;
      quickRepeaterMusicBufferPosition = Global::musicBufferPosition;
      quickRepeaterMusicRemainingLength = Global::musicBufferRemainingLength;
    }
    else
    {
      quickRepeaterEndTime = Global::musicTimeElapsed;
    }

    f32 diff = quickRepeaterEndTime - quickRepeaterBeginTime;
    if (diff >= 0 && diff < 1.0f) // reset quick repeat
    {
      quickRepeaterBeginTime = 0.0f;
      quickRepeaterEndTime = 0.0f;
    }
  }
  else if (quickRepeaterEndTime > 0.0f && quickRepeaterEndTime < Global::musicTimeElapsed)
  {
    Global::musicTimeElapsed = quickRepeaterBeginTime;

    Global::musicBufferPosition = quickRepeaterMusicBufferPosition;
    Global::musicBufferRemainingLength = quickRepeaterMusicRemainingLength;
  }
}

void Player::tick()
{
  Global::musicTimeElapsed += (Global::frameDelta / 1000.0f) * Global::musicSpeedMultiplier;

  if (playNextTick)
  {
    Global::musicBufferPosition = Global::musicBuffer;
    Global::musicBufferRemainingLength = Global::musicBufferLength;
    if (!previewPlaying)
    {
      Global::musicTimeElapsed = 0.0f;
      Global::inputEsc.toggle = !Global::inputEsc.toggle;
    }
    Sound::pauseAudioDevice(false);
    playNextTick = false;
  }

  if (!previewPlaying)
  {
    quickRepeater();
  }

#ifdef __EMSCRIPTEN__
  if (static bool firstRun; !firstRun)
  {
    playSongEmscripten();
    firstRun = !firstRun;
  }
#endif // __EMSCRIPTEN__
}

void Player::playSong(const Psarc::Info& psarcInfo, InstrumentFlags instrumentFlags)
{
  previewPlaying = false;
  Song::loadSongInfoComplete(psarcInfo, Global::songInfos[Global::songSelected]);

  Global::songTrack = Song::loadTrack(psarcInfo, instrumentFlags);
  Global::songVocals = Song::loadVocals(psarcInfo);
  loadAudio(psarcInfo, false);
}

void Player::playPreview(const Psarc::Info& psarcInfo)
{
  previewPlaying = true;
  loadAudio(psarcInfo, true);
}

void Player::stop()
{

}
