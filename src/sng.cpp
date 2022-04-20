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

  return sngInfo;
}
