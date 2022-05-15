#include "player.h"

#include "global.h"
#include "helper.h"
#include "psarc.h"
#include "song.h"
#include "sound.h"

static bool playNextTick = false;

static void playSongEmscripten()
{
  const std::vector<u8> psarcData = Psarc::readPsarcData(EMSC_PATH(songs / test.psarc));

  const Psarc::Info psarcInfo = Psarc::parse(psarcData);
  Global::songInfo = Song::loadSongInfoManifestOnly(psarcInfo);
  Song::loadSongInfoComplete(psarcInfo, Global::songInfo);

  Global::songTrack = Song::loadTrack(psarcInfo, InstrumentFlags::LeadGuitar);
  Global::songVocals = Song::loadVocals(psarcInfo);

  Psarc::loadOgg(psarcInfo, false);
  Sound::playOgg();

  playNextTick = true;
}

void Player::tick()
{
  if (playNextTick)
  {
    Sound::playOgg();
    Global::inputEsc.toggle = !Global::inputEsc.toggle;
    playNextTick = false;
  }

#ifdef __EMSCRIPTEN__
  if (static bool firstRun; !firstRun)
  {
    playSongEmscripten();
    firstRun = !firstRun;
  }
#endif // __EMSCRIPTEN__
}

void Player::playSong(const Psarc::Info& psarcInfo)
{
  Global::songInfo = Song::loadSongInfoManifestOnly(psarcInfo);
  Song::loadSongInfoComplete(psarcInfo, Global::songInfo);

  Global::songTrack = Song::loadTrack(psarcInfo, InstrumentFlags::LeadGuitar);
  Global::songVocals = Song::loadVocals(psarcInfo);

  Psarc::loadOgg(psarcInfo, false);
  playNextTick = true;
}



void Player::stop()
{

}
