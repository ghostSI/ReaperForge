#include "midi.h"

#include "global.h"

#ifdef SUPPORT_MIDI

#include <conio.h>     /* include for kbhit() and getch() functions */
#include <stdio.h>     /* for printf() function */

#include <windows.h>   /* required before including mmsystem.h */
#include <mmsystem.h>  /* multimedia functions (such as MIDI) for Windows */

#include <set>

static HMIDIIN inputDevice[Const::midiMaxDeviceCount];

void autoConnectDevices(const std::string& deviceNames)
{
  std::vector<std::string> v = string::split(deviceNames, ',');
  for (const std::string& name : v)
  {
    for (i32 i = 0; i < Const::midiMaxDeviceCount; ++i)
    {
      if (name == Global::midiDeviceNames[i])
      {
        Midi::openDevice(i);
        continue;
      }
    }
  }
}

void Midi::init()
{
  Global::midiDeviceCount = midiInGetNumDevs();
  assert(Global::midiDeviceCount <= Const::midiMaxDeviceCount);

  for (u32 i = 0; i < Global::midiDeviceCount; ++i)
  {
    MIDIINCAPS inputCapabilities;
    midiInGetDevCaps(i, &inputCapabilities, sizeof(inputCapabilities));
    Global::midiDeviceNames[i] = inputCapabilities.szPname;
  }

  autoConnectDevices(Global::settings.autoConnectDevices);
}

void Midi::fini()
{
  std::set<std::string> deviceNames;

  for (i32 i = 0; i < Const::midiMaxDeviceCount; ++i)
  {
    if (Global::connectedDevices[i] != 0)
    {
      Midi::closeDevice(i);
      deviceNames.insert(Global::midiDeviceNames[i]);
    }
  }

  Global::settings.autoConnectDevices.clear();
  for (const std::string& name : deviceNames)
    Global::settings.autoConnectDevices += name + ',';
  if (Global::settings.autoConnectDevices.size() >= 1)
    Global::settings.autoConnectDevices.pop_back();
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
  switch (wMsg)
  {
  case MIM_OPEN:
    break;
  case MIM_CLOSE:
    break;
  case MIM_DATA:
  {
    DWORD type = dwParam1 & 0xFF;

    u8 status = dwParam1 & 0xFF;
    u8 data1 = (dwParam1 >> 8) & 0xFF; // noteNumber
    u8 data2 = (dwParam1 >> 16) & 0xFF; // velocity

    Global::midiLearnNote = data1;

    controlVolume(data1, data2);
  }
  break;
  case MIM_LONGDATA:
    //assert(false);
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
  MMRESULT flag = midiInOpen(&inputDevice[index], index, (DWORD_PTR)(void*)MidiInProc, 0, CALLBACK_FUNCTION);
  assert(flag == MMSYSERR_NOERROR);

  MIDIHDR midiHeader{};
  static char SysXBuffer[256] = {};
  midiHeader.lpData = &SysXBuffer[0];
  midiHeader.dwBufferLength = sizeof(SysXBuffer);

  flag = midiInPrepareHeader(inputDevice[index], &midiHeader, sizeof(midiHeader));
  assert(flag == MMSYSERR_NOERROR);

  flag = midiInAddBuffer(inputDevice[index], &midiHeader, sizeof(midiHeader));
  assert(flag == MMSYSERR_NOERROR);

  flag = midiInStart(inputDevice[index]);
  assert(flag == MMSYSERR_NOERROR);

  Global::connectedDevices[index] = true;
}

void Midi::closeDevice(i32 index)
{
  MMRESULT flag = midiInStop(inputDevice[index]);
  assert(flag == MMSYSERR_NOERROR);

  MIDIHDR midiHeader{};
  flag = midiInUnprepareHeader(inputDevice[index], &midiHeader, sizeof(midiHeader));
  assert(flag == MMSYSERR_NOERROR);

  flag = midiInClose(inputDevice[index]);
  assert(flag == MMSYSERR_NOERROR);

  Global::connectedDevices[index] = false;
}

#endif // SUPPORT_MIDI
