#include "midi.h"

#include "global.h"

#ifdef SUPPORT_MIDI

#include <conio.h>     /* include for kbhit() and getch() functions */
#include <stdio.h>     /* for printf() function */

#include <windows.h>   /* required before including mmsystem.h */
#include <mmsystem.h>  /* multimedia functions (such as MIDI) for Windows */

#include <thread>

void Midi::init()
{
  const u32 deviceCount = midiInGetNumDevs();
  Global::midiDeviceNames.resize(deviceCount);

  for (u32 i = 0; i < deviceCount; ++i)
  {
    MIDIINCAPS inputCapabilities;
    midiInGetDevCaps(i, &inputCapabilities, sizeof(inputCapabilities));
    Global::midiDeviceNames[i] = inputCapabilities.szPname;
  }
}

static void controlVolume(const u8 noteNumber, const u8 velocity)
{
  switch (noteNumber)
  {
    case 1:
      Global::settings.mixerMusicVolume = velocity;
      break;
    case 2:
      Global::settings.mixerGuitar1Volume = velocity;
      break;
    case 3:
      Global::settings.mixerBass1Volume = velocity;
      break;
    case 4:
      Global::settings.mixerGuitar2Volume = velocity;
      break;
    case 5:
      Global::settings.highwayBackgroundColor.v0 = f32(velocity) / 127.0f; // missing mutex
      break;
    case 6:
      Global::settings.highwayBackgroundColor.v1 = f32(velocity) / 127.0f;
      break;
    case 7:
      Global::settings.highwayBackgroundColor.v2 = f32(velocity) / 127.0f;
      break;
    case 8:
      Global::settings.highwayBackgroundColor.v3 = f32(velocity) / 127.0f;
      break;
  }
}

static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
  char buffer[80] = {};
  LPMIDIHDR lpMIDIHeader;
  u8* ptr;
  u8 bytes;

  switch (wMsg)
  {
  case MIM_OPEN:
    break;
  case MIM_CLOSE:
    assert(false);
    break;
  case MIM_DATA:
  {
    DWORD type = dwParam1 & 0xFF;

    u8 status = dwParam1 & 0xFF;
    u8 data1 = (dwParam1 >> 8) & 0xFF; // noteNumber
    u8 data2 = (dwParam1 >> 16) & 0xFF; // velocity

    controlVolume(data1, data2);
  }
  break;
  case MIM_LONGDATA:
    assert(false);
    break;
  case MIM_ERROR:
    assert(false);
    break;
  case MIM_LONGERROR:
    //assert(false); // there is an error because midi device is not closed on exit
    break;
  case MIM_MOREDATA:
    assert(false);
    break;
  default:
    assert(false);
    break;
  }
  return;
}

void Midi::openDevice(i32 index)
{
  static HMIDIIN inputDevice;
  const MMRESULT flag = midiInOpen(&inputDevice, index, (DWORD_PTR)(void*)MidiInProc, 0, CALLBACK_FUNCTION);
  assert(flag == MMSYSERR_NOERROR);

  MIDIHDR midiHeader{};

  static char SysXBuffer[256] = {};
  midiHeader.lpData = &SysXBuffer[0];
  midiHeader.dwBufferLength = sizeof(SysXBuffer);

  i32 res = midiInPrepareHeader(inputDevice, &midiHeader, sizeof(midiHeader));
  assert(res == MMSYSERR_NOERROR);

  res = midiInAddBuffer(inputDevice, &midiHeader, sizeof(midiHeader));
  assert(res == MMSYSERR_NOERROR);

  res = midiInStart(inputDevice);
  assert(res == MMSYSERR_NOERROR);
}

#endif // SUPPORT_MIDI
