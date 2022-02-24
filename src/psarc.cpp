#include "psarc.h"

#include "file.h"

static u32 u32BigEndian(const u8* bytes)
{
    return bytes[0] << 24 | bytes[1] << 16 | bytes [2] << 8 | bytes[3];
}

std::vector<u8> Psarc::readPsarcData(const char* filepath)
{
    const size_t filepathLen = strlen(filepath);

    ASSERT(filepathLen > sizeof("_p.psarc") && "Psarc filename invalid");
    ASSERT(strncmp(filepath, "_p.psarc", filepathLen - sizeof("_p.psarc")));

    std::vector<u8> psarcData = File::load(filepath, "r");

    ASSERT(strncmp(reinterpret_cast<char*>(psarcData.data()), "PSAR", sizeof("PSAR")) == 0 && "Invalid Psarc content");

    return psarcData;
}

Psarc::PsarcInfo Psarc::read(const std::vector<u8>& psarcData)
{
    PsarcInfo psarcInfo;

    psarcInfo.header.magicNumber = u32BigEndian(&psarcData[0]);
    ASSERT(psarcInfo.header.magicNumber == 1347633490_u32 && "Invalid Psarc content");
    psarcInfo.header.version = u32BigEndian(&psarcData[4]);
    psarcInfo.header.compressMethod = u32BigEndian(&psarcData[8]);
    psarcInfo.header.totalTocSize = u32BigEndian(&psarcData[12]);
    psarcInfo.header.TOCEntrySize = u32BigEndian(&psarcData[16]);
    psarcInfo.header.numFiles = u32BigEndian(&psarcData[20]);
    psarcInfo.header.BlockSizeAlloc = u32BigEndian(&psarcData[240]);
    psarcInfo.header.archiveFlags = u32BigEndian(&psarcData[28]);

    return  psarcInfo;
}