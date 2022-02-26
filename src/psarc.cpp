#include "psarc.h"

#include "file.h"
#include "rijndael.h"

static constexpr const char *psarcExtension = "_p.psarc";
static constexpr const char *magicText = "PSAR";

static u32 u32BigEndian(const u8 *bytes) {
    return bytes[0] << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3];
}

std::vector<u8> Psarc::readPsarcData(const char *filepath) {
    const size_t filepathLen = strlen(filepath);

    ASSERT(filepathLen > sizeof(psarcExtension) && "Psarc filename invalid");
    ASSERT(strncmp(filepath, psarcExtension, filepathLen - sizeof(psarcExtension)));

    std::vector<u8> psarcData = File::load(filepath, "r");

    ASSERT(strncmp(reinterpret_cast<char *>(psarcData.data()), magicText, sizeof(magicText)) == 0 &&
           "Invalid Psarc content");

    return psarcData;
}

static const u8 psarcKey[32] =
        {
                0xC5, 0x3D, 0xB2, 0x38, 0x70, 0xA1, 0xA2, 0xF7,
                0x1C, 0xAE, 0x64, 0x06, 0x1F, 0xDD, 0x0E, 0x11,
                0x57, 0x30, 0x9D, 0xC8, 0x52, 0x04, 0xD4, 0xC5,
                0xBF, 0xDF, 0x25, 0x09, 0x0D, 0xF2, 0x57, 0x2C
        };

static std::vector<u8> decryptPsarc(const u8 *bytes, const u32 len) {
    int lea = 4096;


    std::vector<u8> plainText(lea);

    Rijndael::decrypt(psarcKey, bytes, plainText.data(), lea);

    auto a = plainText[1503];

    return plainText;
}

Psarc::PsarcInfo Psarc::read(const std::vector<u8> &psarcData) {
    PsarcInfo psarcInfo;

    psarcInfo.header.magicNumber = u32BigEndian(&psarcData[0]);
    ASSERT(psarcInfo.header.magicNumber == 1347633490_u32 && "Invalid Psarc content");
    psarcInfo.header.version = u32BigEndian(&psarcData[4]);
    psarcInfo.header.compressMethod = u32BigEndian(&psarcData[8]);
    psarcInfo.header.totalTocSize = u32BigEndian(&psarcData[12]);
    psarcInfo.header.TOCEntrySize = u32BigEndian(&psarcData[16]);
    psarcInfo.header.numFiles = u32BigEndian(&psarcData[20]);
    psarcInfo.header.BlockSizeAlloc = u32BigEndian(&psarcData[24]);
    psarcInfo.header.archiveFlags = u32BigEndian(&psarcData[28]);

    decryptPsarc(&psarcData[32], psarcInfo.header.totalTocSize);

    return psarcInfo;
}