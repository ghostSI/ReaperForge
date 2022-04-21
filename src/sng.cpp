#include "sng.h"

#include "rijndael.h"
#include "inflate.h"

static const u8 sngKey[32] = {
  0xCB, 0x64, 0x8D, 0xF3, 0xD1, 0x2A, 0x16, 0xBF,
  0x71, 0x70, 0x14, 0x14, 0xE6, 0x96, 0x19, 0xEC,
  0x17, 0x1C, 0xCA, 0x5D, 0x2A, 0x14, 0x2E, 0x3E,
  0x59, 0xDE, 0x7A, 0xDD, 0xA1, 0x8A, 0x3A, 0x30
};

static u16 u16LittleEndian(const u8* bytes)
{
  return *reinterpret_cast<const u16*>(bytes);
}

static i16 i16LittleEndian(const u8* bytes)
{
  return *reinterpret_cast<const i16*>(bytes);
}

static u32 u32LittleEndian(const u8* bytes)
{
  return *reinterpret_cast<const u32*>(bytes);
}

static i32 i32LittleEndian(const u8* bytes)
{
  return *reinterpret_cast<const i32*>(bytes);
}

static f32 f32LittleEndian(const u8* bytes)
{
  return *reinterpret_cast<const f32*>(bytes);
}

static f64 f64LittleEndian(const u8* bytes)
{
  return *reinterpret_cast<const f64*>(bytes);
}

static std::vector<u8> decryptSngData(const std::vector<u8>& sngData)
{
  const u32 magicNumber = u32LittleEndian(&sngData[0]);

  assert(magicNumber == 0x4A);

  u8 iv[16];
  memcpy(iv, &sngData[8], sizeof(iv));

  const i64 len = sngData.size() - 24;
  std::vector<u8> decrypedData(len);
  for (i64 i = 0; i < len; i += 16)
  {
    Rijndael::decrypt(sngKey, &sngData[24 + i], &decrypedData[i], 16, iv);

    {
      bool carry = false;
      for (i64 j = (sizeof(iv)) - 1, carry = true; j >= 0 && carry; j--)
        carry = ((iv[j] = (u8)(iv[j] + 1)) == 0);
    }
  }

  return decrypedData;
}

static std::vector<u8> inflateSngPlainText(const std::vector<u8>& decrypedSngData)
{
  const i32 plainTextLen = u32LittleEndian(&decrypedSngData[0]);
  const u16 zHeader = u16LittleEndian(&decrypedSngData[4]);
  assert(zHeader == 0x78DA || zHeader == 0xDA78); //LE 55928 //BE 30938

  std::vector<u8> plainText(plainTextLen);
  Inflate::inflate(&decrypedSngData[4], decrypedSngData.size() - 4, &plainText[0], plainTextLen);

  return plainText;
}

Sng::Info Sng::parse(const std::vector<u8>& sngData)
{
  Sng::Info sngInfo;

  const std::vector<u8> decrypedSngData = decryptSngData(sngData);
  const std::vector<u8> plainText = inflateSngPlainText(decrypedSngData);

  u64 j = 0;
  {
    const i32 bpmCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.bpm.resize(bpmCount);
    for (i32 i = 0; i < bpmCount; ++i)
    {
      sngInfo.bpm[i].time = f32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.bpm[i].measure = i16LittleEndian(&plainText[j]);
      j += 2;
      sngInfo.bpm[i].beat = i16LittleEndian(&plainText[j]);
      j += 2;
      sngInfo.bpm[i].phraseIteration = i32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.bpm[i].mask = i32LittleEndian(&plainText[j]);
      j += 4;
    }
  }

  {
    const i32 phraseCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.phrase.resize(phraseCount);
    for (i32 i = 0; i < phraseCount; ++i)
    {
      sngInfo.phrase[i].solo = plainText[j];
      j += 1;
      sngInfo.phrase[i].disparity = plainText[j];
      j += 1;
      sngInfo.phrase[i].ignore = plainText[j];
      j += 1;
      sngInfo.phrase[i].paddin = plainText[j];
      j += 1;
      sngInfo.phrase[i].maxDifficulty = i32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.phrase[i].phraseIterationLinks = i32LittleEndian(&plainText[j]);
      j += 4;
      memcpy(&sngInfo.phrase[i].name, &plainText[j], 32);
      j += 32;
    }
  }

  {
    const i32 chordCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.chord.resize(chordCount);
    for (i32 i = 0; i < chordCount; ++i)
    {
      sngInfo.chord[i].mask = plainText[j];
      j += 4;
      memcpy(&sngInfo.chord[i].frets, &plainText[j], 6);
      j += 6;
      memcpy(&sngInfo.chord[i].fingers, &plainText[j], 6);
      j += 6;
      memcpy(&sngInfo.chord[i].notes, &plainText[j], 24);
      j += 24;
      memcpy(&sngInfo.chord[i].name, &plainText[j], 32);
      j += 32;
    }
  }

  {
    const i32 chordNotesCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.chordNotes.resize(chordNotesCount);
    for (i32 i = 0; i < chordNotesCount; ++i)
    {
      memcpy(&sngInfo.chordNotes[i].noteMask, &plainText[j], 24);
      j += 24;
      memcpy(&sngInfo.chordNotes[i].bendData, &plainText[j], 2328);
      j += 2328;
      memcpy(&sngInfo.chordNotes[i].slideTo, &plainText[j], 6);
      j += 6;
      memcpy(&sngInfo.chordNotes[i].slideUnpitchTo, &plainText[j], 6);
      j += 6;
      memcpy(&sngInfo.chordNotes[i].vibrato, &plainText[j], 12);
      j += 12;
    }
  }

  {
    const i32 vocalCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.vocal.resize(vocalCount);
    for (i32 i = 0; i < vocalCount; ++i)
    {
      sngInfo.vocal[i].time = f32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.vocal[i].note = i32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.vocal[i].length = f32LittleEndian(&plainText[j]);
      j += 4;
      memcpy(&sngInfo.vocal[i].lyric, &plainText[j], 48);
      j += 48;
    }
  }

  if (sngInfo.vocal.size() > 0)
  {
    {
      const i32 symbolsHeaderCount = u32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.symbolsHeader.resize(symbolsHeaderCount);
      for (i32 i = 0; i < symbolsHeaderCount; ++i)
      {
        sngInfo.symbolsHeader[i].unk1 = i32LittleEndian(&plainText[j]);
        j += 4;
        sngInfo.symbolsHeader[i].unk2 = i32LittleEndian(&plainText[j]);
        j += 4;
        sngInfo.symbolsHeader[i].unk3 = i32LittleEndian(&plainText[j]);
        j += 4;
        sngInfo.symbolsHeader[i].unk4 = i32LittleEndian(&plainText[j]);
        j += 4;
        sngInfo.symbolsHeader[i].unk5 = i32LittleEndian(&plainText[j]);
        j += 4;
        sngInfo.symbolsHeader[i].unk6 = i32LittleEndian(&plainText[j]);
        j += 4;
        sngInfo.symbolsHeader[i].unk7 = i32LittleEndian(&plainText[j]);
        j += 4;
        sngInfo.symbolsHeader[i].unk8 = i32LittleEndian(&plainText[j]);
        j += 4;
      }
    }

    {
      const i32 symbolsTextureCount = u32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.symbolsTexture.resize(symbolsTextureCount);
      for (i32 i = 0; i < symbolsTextureCount; ++i)
      {
        memcpy(&sngInfo.symbolsTexture[i].font, &plainText[j], 128);
        j += 128;
        sngInfo.symbolsTexture[i].fontpathLength = i32LittleEndian(&plainText[j]);
        j += 4;
        sngInfo.symbolsTexture[i].unk10 = i32LittleEndian(&plainText[j]);
        j += 4;
        sngInfo.symbolsTexture[i].width = i32LittleEndian(&plainText[j]);
        j += 4;
        sngInfo.symbolsTexture[i].height = i32LittleEndian(&plainText[j]);
        j += 4;
      }
    }

    {
      const i32 symbolDefinitionCount = u32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.symbolDefinition.resize(symbolDefinitionCount);
      for (i32 i = 0; i < symbolDefinitionCount; ++i)
      {
        memcpy(&sngInfo.symbolDefinition[i].text, &plainText[j], 12);
        j += 12;
        memcpy(&sngInfo.symbolDefinition[i].rectOutter, &plainText[j], 16);
        j += 16;
        memcpy(&sngInfo.symbolDefinition[i].rectInner, &plainText[j], 16);
        j += 16;
      }
    }
  }

  {
    const i32 phraseIterationCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.phraseIteration.resize(phraseIterationCount);
    for (i32 i = 0; i < phraseIterationCount; ++i)
    {
      sngInfo.phraseIteration[i].phraseId = i32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.phraseIteration[i].startTime = f32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.phraseIteration[i].nextPhraseTime = f32LittleEndian(&plainText[j]);
      j += 4;
      memcpy(&sngInfo.phraseIteration[i].difficulty, &plainText[j], 12);
      j += 12;
    }
  }

  {
    const i32 phraseExtraInfoByLevelCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.phraseExtraInfoByLevel.resize(phraseExtraInfoByLevelCount);
    for (i32 i = 0; i < phraseExtraInfoByLevelCount; ++i)
    {
      sngInfo.phraseExtraInfoByLevel[i].phraseId = i32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.phraseExtraInfoByLevel[i].difficulty = i32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.phraseExtraInfoByLevel[i].empty = i32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.phraseExtraInfoByLevel[i].levelJump = plainText[j];
      j += 1;
      sngInfo.phraseExtraInfoByLevel[i].redundant = i16LittleEndian(&plainText[j]);
      j += 2;
      sngInfo.phraseExtraInfoByLevel[i].padding = plainText[j];
      j += 1;
    }
  }

  {
    const i32 nLinkedDifficultyCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.nLinkedDifficulty.resize(nLinkedDifficultyCount);
    for (i32 i = 0; i < nLinkedDifficultyCount; ++i)
    {
      sngInfo.nLinkedDifficulty[i].levelBreak = i32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.nLinkedDifficulty[i].phraseCount = i32LittleEndian(&plainText[j]);
      j += 4;
      const i32 nLDPhraseCount = u32LittleEndian(&plainText[j]);
      j += 4;
      {
        sngInfo.nLinkedDifficulty[i].nLDPhrase.resize(nLDPhraseCount);
        for (i32 ii = 0; ii < nLDPhraseCount; ++ii)
        {
          sngInfo.nLinkedDifficulty[i].nLDPhrase[ii] = i32LittleEndian(&plainText[j]);
          j += 4;
        }
      }
    }
  }

  {
    const i32 actionCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.action.resize(actionCount);
    for (i32 i = 0; i < actionCount; ++i)
    {
      sngInfo.action[i].time = f32LittleEndian(&plainText[j]);
      j += 4;
      memcpy(&sngInfo.action[i].actionName, &plainText[j], 256);
      j += 256;
    }
  }

  {
    const i32 eventCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.event.resize(eventCount);
    for (i32 i = 0; i < eventCount; ++i)
    {
      sngInfo.event[i].time = f32LittleEndian(&plainText[j]);
      j += 4;
      memcpy(&sngInfo.event[i].eventName, &plainText[j], 256);
      j += 256;
    }
  }

  {
    const i32 toneCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.tone.resize(toneCount);
    for (i32 i = 0; i < toneCount; ++i)
    {
      sngInfo.tone[i].time = f32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.tone[i].toneId = i32LittleEndian(&plainText[j]);
      j += 4;
    }
  }

  {
    const i32 dnaCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.dna.resize(dnaCount);
    for (i32 i = 0; i < dnaCount; ++i)
    {
      sngInfo.dna[i].time = f32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.dna[i].dnaId = i32LittleEndian(&plainText[j]);
      j += 4;
    }
  }

  {
    const i32 sectionCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.section.resize(sectionCount);
    for (i32 i = 0; i < sectionCount; ++i)
    {
      memcpy(&sngInfo.section[i].name, &plainText[j], 32);
      j += 32;
      sngInfo.section[i].number = i32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.section[i].startTime = f32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.section[i].endTime = f32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.section[i].startPhraseIterationId = i32LittleEndian(&plainText[j]);
      j += 4;
      sngInfo.section[i].endPhraseIterationId = i32LittleEndian(&plainText[j]);
      j += 4;
      memcpy(&sngInfo.section[i].stringMask, &plainText[j], 36);
      j += 36;
    }
  }

  {
    const i32 arrangementCount = u32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.arrangement.resize(arrangementCount);
    for (i32 i = 0; i < arrangementCount; ++i)
    {
      sngInfo.arrangement[i].difficulty = i32LittleEndian(&plainText[j]);
      j += 4;
      const i32 anchorCount = u32LittleEndian(&plainText[j]);
      j += 4;
      {
        sngInfo.arrangement[i].anchors.resize(anchorCount);
        for (i32 ii = 0; ii < anchorCount; ++ii)
        {
          sngInfo.arrangement[i].anchors[ii].startBeatTime = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].anchors[ii].endBeatTime = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].anchors[ii].unk3_FirstNoteTime = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].anchors[ii].unk4_LastNoteTime = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].anchors[ii].fretId = plainText[j];
          j += 1;
          memcpy(&sngInfo.arrangement[i].anchors[ii].padding, &plainText[j], 3);
          j += 3;
          sngInfo.arrangement[i].anchors[ii].width = i32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].anchors[ii].phraseIterationId = i32LittleEndian(&plainText[j]);
          j += 4;
        }
      }
      const i32 anchorExtensionCount = u32LittleEndian(&plainText[j]);
      j += 4;
      {
        sngInfo.arrangement[i].anchorExtensions.resize(anchorExtensionCount);
        for (i32 ii = 0; ii < anchorExtensionCount; ++ii)
        {
          sngInfo.arrangement[i].anchorExtensions[ii].beatTime = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].anchorExtensions[ii].fretId = plainText[j];
          j += 1;
          sngInfo.arrangement[i].anchorExtensions[ii].unk2_0 = i32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].anchorExtensions[ii].unk3_0 = i16LittleEndian(&plainText[j]);
          j += 2;
          sngInfo.arrangement[i].anchorExtensions[ii].unk4_0 = plainText[j];
          j += 1;
        }
      }
      const i32 fingerprints1Count = u32LittleEndian(&plainText[j]);
      j += 4;
      {
        sngInfo.arrangement[i].fingerprints1.resize(fingerprints1Count);
        for (i32 ii = 0; ii < fingerprints1Count; ++ii)
        {
          sngInfo.arrangement[i].fingerprints1[ii].chordId = i32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].fingerprints1[ii].startTime = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].fingerprints1[ii].endTime = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].fingerprints1[ii].unk3_FirstNoteTime = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].fingerprints1[ii].unk4_LastNoteTime = f32LittleEndian(&plainText[j]);
          j += 4;
        }
      }
      const i32 fingerprints2Count = u32LittleEndian(&plainText[j]);
      j += 4;
      {
        sngInfo.arrangement[i].fingerprints2.resize(fingerprints2Count);
        for (i32 ii = 0; ii < fingerprints2Count; ++ii)
        {
          sngInfo.arrangement[i].fingerprints2[ii].chordId = i32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].fingerprints2[ii].startTime = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].fingerprints2[ii].endTime = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].fingerprints2[ii].unk3_FirstNoteTime = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].fingerprints2[ii].unk4_LastNoteTime = f32LittleEndian(&plainText[j]);
          j += 4;
        }
      }
      const i32 noteCount = u32LittleEndian(&plainText[j]);
      j += 4;
      {
        sngInfo.arrangement[i].notes.resize(noteCount);
        for (i32 ii = 0; ii < noteCount; ++ii)
        {
          sngInfo.arrangement[i].notes[ii].noteMask = u32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].notes[ii].noteFlags = u32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].notes[ii].hash = u32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].notes[ii].time = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].notes[ii].stringIndex = plainText[j];
          j += 1;
          sngInfo.arrangement[i].notes[ii].fretId = plainText[j];
          j += 1;
          sngInfo.arrangement[i].notes[ii].anchorFretId = plainText[j];
          j += 1;
          sngInfo.arrangement[i].notes[ii].anchorWidth = plainText[j];
          j += 1;
          sngInfo.arrangement[i].notes[ii].chordId = i32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].notes[ii].chordNotesId = i32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].notes[ii].phraseId = i32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].notes[ii].phraseIterationId = i32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].notes[ii].fingerPrintId[0] = i16LittleEndian(&plainText[j]);
          j += 2;
          sngInfo.arrangement[i].notes[ii].fingerPrintId[1] = i16LittleEndian(&plainText[j]);
          j += 2;
          sngInfo.arrangement[i].notes[ii].nextIterNote = i16LittleEndian(&plainText[j]);
          j += 2;
          sngInfo.arrangement[i].notes[ii].prevIterNote = i16LittleEndian(&plainText[j]);
          j += 2;
          sngInfo.arrangement[i].notes[ii].parentPrevNote = i16LittleEndian(&plainText[j]);
          j += 2;
          sngInfo.arrangement[i].notes[ii].slideTo = plainText[j];
          j += 1;
          sngInfo.arrangement[i].notes[ii].slideUnpitchTo = plainText[j];
          j += 1;
          sngInfo.arrangement[i].notes[ii].leftHand = plainText[j];
          j += 1;
          sngInfo.arrangement[i].notes[ii].tap = plainText[j];
          j += 1;
          sngInfo.arrangement[i].notes[ii].pickDirection = plainText[j];
          j += 1;
          sngInfo.arrangement[i].notes[ii].slap = plainText[j];
          j += 1;
          sngInfo.arrangement[i].notes[ii].pluck = plainText[j];
          j += 1;
          sngInfo.arrangement[i].notes[ii].vibrato = i16LittleEndian(&plainText[j]);
          j += 2;
          sngInfo.arrangement[i].notes[ii].sustain = f32LittleEndian(&plainText[j]);
          j += 4;
          sngInfo.arrangement[i].notes[ii].maxBend = f32LittleEndian(&plainText[j]);
          j += 4;
          const i32 bendDataCount = u32LittleEndian(&plainText[j]);
          j += 4;
          {
            sngInfo.arrangement[i].notes[ii].bendData.resize(bendDataCount);
            for (i32 ii = 0; ii < bendDataCount; ++ii)
            {
              memcpy(&sngInfo.arrangement[i].notes[ii].bendData, &plainText[j], 2328);
              j += 2328;
            }
          }
        }
      }
      sngInfo.arrangement[i].phraseCount = i32LittleEndian(&plainText[j]);
      j += 4;
      {
        sngInfo.arrangement[i].averageNotesPerIteration.resize(sngInfo.arrangement[i].phraseCount);
        for (i32 ii = 0; ii < sngInfo.arrangement[i].phraseCount; ++ii)
        {
          sngInfo.arrangement[i].averageNotesPerIteration[ii] = f32LittleEndian(&plainText[j]);
          j += 4;
        }
      }
      sngInfo.arrangement[i].phraseIterationCount1 = i32LittleEndian(&plainText[j]);
      j += 4;
      {
        sngInfo.arrangement[i].notesInIteration1.resize(sngInfo.arrangement[i].phraseIterationCount1);
        for (i32 ii = 0; ii < sngInfo.arrangement[i].phraseIterationCount1; ++ii)
        {
          sngInfo.arrangement[i].notesInIteration1[ii] = i32LittleEndian(&plainText[j]);
          j += 4;
        }
      }
      sngInfo.arrangement[i].phraseIterationCount2 = i32LittleEndian(&plainText[j]);
      j += 4;
      {
        sngInfo.arrangement[i].notesInIteration2.resize(sngInfo.arrangement[i].phraseIterationCount2);
        for (i32 ii = 0; ii < sngInfo.arrangement[i].phraseIterationCount2; ++ii)
        {
          sngInfo.arrangement[i].notesInIteration2[ii] = i32LittleEndian(&plainText[j]);
          j += 4;
        }
      }
    }
  }

  {
    sngInfo.metadata.maxScore = f64LittleEndian(&plainText[j]);
    j += 8;
    sngInfo.metadata.maxNotesAndChords = f64LittleEndian(&plainText[j]);
    j += 8;
    sngInfo.metadata.maxNotesAndChordsReal = f64LittleEndian(&plainText[j]);
    j += 8;
    sngInfo.metadata.pointsPerNote = f64LittleEndian(&plainText[j]);
    j += 8;
    sngInfo.metadata.firstBeatLength = f32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.metadata.startTime = f32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.metadata.capoFretId = plainText[j];
    j += 1;
    memcpy(&sngInfo.metadata.lastConversionDateTime, &plainText[j], 32);
    j += 32;
    sngInfo.metadata.part = i16LittleEndian(&plainText[j]);
    j += 2;
    sngInfo.metadata.songLength = f32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.metadata.stringCount = i32LittleEndian(&plainText[j]);
    j += 4;
    {
      sngInfo.metadata.tuning.resize(sngInfo.metadata.stringCount);
      for (i32 ii = 0; ii < sngInfo.metadata.stringCount; ++ii)
      {
        sngInfo.metadata.tuning[ii] = i16LittleEndian(&plainText[j]);
        j += 2;
      }
    }
    sngInfo.metadata.unk11FirstNoteTime = f32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.metadata.unk12FirstNoteTime = f32LittleEndian(&plainText[j]);
    j += 4;
    sngInfo.metadata.maxDifficulty = i32LittleEndian(&plainText[j]);
    j += 4;
  }

  return sngInfo;
}
