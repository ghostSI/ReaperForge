#include "midi.h"

#ifdef SUPPORT_MIDI

#include "global.h"


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

  for (i32 i = 0; i < Global::midiDeviceCount; ++i)
  {
    MIDIINCAPS inputCapabilities;
    midiInGetDevCaps(i, &inputCapabilities, sizeof(inputCapabilities));
#ifdef UNICODE
    char ch[MAXPNAMELEN];
    const char DefChar = ' ';
    WideCharToMultiByte(CP_ACP, 0, inputCapabilities.szPname, -1, ch, MAXPNAMELEN, &DefChar, NULL);
    Global::midiDeviceNames[i] = ch;
#else // UNICODE
    Global::midiDeviceNames[i] = inputCapabilities.szPname;
#endif // UNICODE
  }

  autoConnectDevices(Global::settings.autoConnectDevices);

  for (i32 i = 0; i < NUM(Global::settings.midiBinding); ++i)
  {

    Global::midiNoteBinding[Global::settings.midiBinding[i]] = i;
  }
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

enum struct MidiBinding {
  AudioSignalChainDec,
  AudioSignalChainInc,
  MixerMusicVolume,
  MixerGuitar1Volume,
  MixerBass1Volume,
  MixerGuitar2Volume,
  HighwayBackgroundColorV0,
  HighwayBackgroundColorV1,
  HighwayBackgroundColorV2,
  ToneAssignment0,
  ToneAssignment1,
  ToneAssignment2,
  ToneAssignment3,
  ToneAssignment4,
  ToneAssignment5,
  ToneAssignment6,
  ToneAssignment7,
  ToneAssignment8,
  ToneAssignment9,
  ToneAssignmentBankInc,
  ToneAssignmentBankDec
};

static void interpretMidiNote(const u8 noteNumber, const u8 velocity)
{
  if (velocity != 127) // for now only footswitches are supported
    return;

  const MidiBinding binding = MidiBinding(Global::midiNoteBinding[noteNumber]);
  if (binding == MidiBinding(0xFF))
    return;

  switch (binding)
  {
  case MidiBinding::AudioSignalChainDec:
    if (to_underlying(Global::settings.audioSignalChain) < to_underlying(SignalChain::COUNT) - 1)
      Global::settings.audioSignalChain = SignalChain(to_underlying(Global::settings.audioSignalChain) + 1);
    break;
  case MidiBinding::AudioSignalChainInc:
    if (to_underlying(Global::settings.audioSignalChain) > 0)
      Global::settings.audioSignalChain = SignalChain(to_underlying(Global::settings.audioSignalChain) - 1);
    break;
  case MidiBinding::MixerMusicVolume:
    Global::settings.mixerMusicVolume = velocity;
    break;
  case MidiBinding::MixerGuitar1Volume:
    Global::settings.mixerGuitar1Volume = velocity;
    break;
  case MidiBinding::MixerBass1Volume:
    Global::settings.mixerBass1Volume = velocity;
    break;
  case MidiBinding::MixerGuitar2Volume:
    Global::settings.mixerGuitar2Volume = velocity;
    break;
  case MidiBinding::HighwayBackgroundColorV0:
    Global::settings.highwayBackgroundColor.v0 = f32(velocity) / 127.0f; // missing mutex
    break;
  case MidiBinding::HighwayBackgroundColorV1:
    Global::settings.highwayBackgroundColor.v1 = f32(velocity) / 127.0f;
    break;
  case MidiBinding::HighwayBackgroundColorV2:
    Global::settings.highwayBackgroundColor.v2 = f32(velocity) / 127.0f;
    break;
  case MidiBinding::ToneAssignment0:
    Global::toneAssignment = (Global::toneAssignment / 10) * 10;
    break;
  case MidiBinding::ToneAssignment1:
    Global::toneAssignment = (Global::toneAssignment / 10) * 10 + 1;
    break;
  case MidiBinding::ToneAssignment2:
    Global::toneAssignment = (Global::toneAssignment / 10) * 10 + 2;
    break;
  case MidiBinding::ToneAssignment3:
    Global::toneAssignment = (Global::toneAssignment / 10) * 10 + 3;
    break;
  case MidiBinding::ToneAssignment4:
    Global::toneAssignment = (Global::toneAssignment / 10) * 10 + 4;
    break;
  case MidiBinding::ToneAssignment5:
    Global::toneAssignment = (Global::toneAssignment / 10) * 10 + 5;
    break;
  case MidiBinding::ToneAssignment6:
    Global::toneAssignment = (Global::toneAssignment / 10) * 10 + 6;
    break;
  case MidiBinding::ToneAssignment7:
    Global::toneAssignment = (Global::toneAssignment / 10) * 10 + 7;
    break;
  case MidiBinding::ToneAssignment8:
    Global::toneAssignment = (Global::toneAssignment / 10) * 10 + 8;
    break;
  case MidiBinding::ToneAssignment9:
    Global::toneAssignment = (Global::toneAssignment / 10) * 10 + 9;
    break;
  case MidiBinding::ToneAssignmentBankDec:
    if (Global::toneAssignment / 10 > 0)
      Global::toneAssignment -= 10;
    break;
  case MidiBinding::ToneAssignmentBankInc:
    if (Global::toneAssignment / 10 < Const::profileToneAssignmentCount / 10 - 1)
      Global::toneAssignment += 10;
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
    DWORD status = dwParam1 & 0xFF;
    switch (status)
    {
    case 144: // Note
    case 176: // CC
    case 192: // Unknown;
      break;
    default:
      assert(false);
      break;
    }

    u8 data1 = (dwParam1 >> 8) & 0xFF; // noteNumber
    u8 data2 = (dwParam1 >> 16) & 0xFF; // velocity

    Global::midiLearnNote = data1;

    interpretMidiNote(data1, data2);
  }
  break;
  case MIM_LONGDATA:
    //assert(false);
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

  flag = midiInReset(inputDevice[index]);
  assert(flag == MMSYSERR_NOERROR);

  flag = midiInClose(inputDevice[index]);
  assert(flag == MMSYSERR_NOERROR);

  Global::connectedDevices[index] = false;
}

#endif // SUPPORT_MIDI
