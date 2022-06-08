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

static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
  switch (wMsg) {
  case MIM_OPEN:
    printf("wMsg=MIM_OPEN\n");
    break;
  case MIM_CLOSE:
    printf("wMsg=MIM_CLOSE\n");
    break;
  case MIM_DATA:
    printf("wMsg=MIM_DATA, dwInstance=%08x, dwParam1=%08x, dwParam2=%08x\n", dwInstance, dwParam1, dwParam2);
    break;
  case MIM_LONGDATA:
    printf("wMsg=MIM_LONGDATA\n");
    break;
  case MIM_ERROR:
    printf("wMsg=MIM_ERROR\n");
    break;
  case MIM_LONGERROR:
    printf("wMsg=MIM_LONGERROR\n");
    break;
  case MIM_MOREDATA:
    printf("wMsg=MIM_MOREDATA\n");
    break;
  default:
    printf("wMsg = unknown\n");
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
