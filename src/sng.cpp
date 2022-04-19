#include "sng.h"

#include "rijndael.h"

static const u8 sngKey[32] = {
  0xCB, 0x64, 0x8D, 0xF3, 0xD1, 0x2A, 0x16, 0xBF,
  0x71, 0x70, 0x14, 0x14, 0xE6, 0x96, 0x19, 0xEC,
  0x17, 0x1C, 0xCA, 0x5D, 0x2A, 0x14, 0x2E, 0x3E,
  0x59, 0xDE, 0x7A, 0xDD, 0xA1, 0x8A, 0x3A, 0x30
};

static u32 u32LittleEndian(const u8* bytes)
{
  return *reinterpret_cast<const u32*>(bytes);
}

Sng::Info Sng::parse(const std::vector<u8>& sngData)
{
  Sng::Info sngInfo;

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

  u8 expectedData[] = { 7,5,89,0,120,218,236,221,125,148,100,101,125,232,251,93,213,187,95,167,103,24,1,67,240,109,26,4,211,136,72,35 };
  for (i32 i = 0; i < sizeof(expectedData); ++i)
  {
    if (i == 16)
    {
      i = i;
    }

    assert(expectedData[i] == decrypedData[i]);
  }

  return sngInfo;
}
