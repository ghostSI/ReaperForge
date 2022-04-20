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

static u32 u32LittleEndian(const u8* bytes)
{
  return *reinterpret_cast<const u32*>(bytes);
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
  const i64 plainTextLen = u32LittleEndian(&decrypedSngData[0]);
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

  return sngInfo;
}
