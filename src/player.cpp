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
  const std::vector<u8> psarcData = Psarc::readPsarcData(EMSC_PATH(songs/test.psarc));

  const Psarc::Info psarcInfo = Psarc::parse(psarcData);
  Global::songInfos.push_back(Song::loadSongInfoManifestOnly(psarcInfo));
  Song::loadSongInfoComplete(psarcInfo, Global::songInfos[Global::songSelected]);

  Global::songTrack = Song::loadTrack(psarcInfo, InstrumentFlags::LeadGuitar);
  Global::songVocals = Song::loadVocals(psarcInfo);

  Sound::pauseAudioDevice(true);
  loadAudio(psarcInfo, false);

  playNextTick = true;
}

void Player::tick()
{
  Global::musicTimeElapsed += (Global::frameDelta / 1000.0f) * Global::musicSpeedMultiplier;

  if (playNextTick)
  {
    Global::musicBufferPosition = Global::musicBuffer;
    Global::musicBufferRemainingLength = Global::musicBufferLength;
    Sound::pauseAudioDevice(false);

    Global::musicTimeElapsed = 0.0f;

    Global::inputEsc.toggle = !Global::inputEsc.toggle;
    playNextTick = false;
  }

  //musicTimeElapsed = f32(Global::musicBufferLength - Global::musicBufferRemainingLength) / f32(Global::settings.audioSampleRate * 4 * 2);

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
  Song::loadSongInfoComplete(psarcInfo, Global::songInfos[Global::songSelected]);

  Global::songTrack = Song::loadTrack(psarcInfo, instrumentFlags);
  Global::songVocals = Song::loadVocals(psarcInfo);
  loadAudio(psarcInfo, false);
}

void Player::stop()
{

}
