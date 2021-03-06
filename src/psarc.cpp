#include "psarc.h"

#include "file.h"
#include "global.h"
#include "inflate.h"
#include "rijndael.h"
#include "wem.h"

#include <string.h>

static const u8 psarcKey[32] = {
        0xC5, 0x3D, 0xB2, 0x38, 0x70, 0xA1, 0xA2, 0xF7,
        0x1C, 0xAE, 0x64, 0x06, 0x1F, 0xDD, 0x0E, 0x11,
        0x57, 0x30, 0x9D, 0xC8, 0x52, 0x04, 0xD4, 0xC5,
        0xBF, 0xDF, 0x25, 0x09, 0x0D, 0xF2, 0x57, 0x2C
};

std::vector<u8> Psarc::readPsarcData(const char *filepath) {
    const size_t filepathLen = strlen(filepath);

    ASSERT(filepathLen > sizeof("_p.psarc") && "Psarc filename invalid");
    ASSERT(strncmp(filepath, "_p.psarc", filepathLen - sizeof("_p.psarc")));

    std::vector<u8> psarcData = File::load(filepath, "rb");

    ASSERT(strncmp(reinterpret_cast<char *>(psarcData.data()), "PSAR", sizeof("PSAR")) == 0 &&
           "Invalid Psarc content");

    return psarcData;
}

static std::vector<u8> decryptPsarc(const std::vector<u8> &psarcData, u32 startPos, const u32 len) {

    // This is a bit strange not sure if the padding stuff is really required.

    std::vector<u8> encText;
    std::copy(psarcData.begin() + startPos, psarcData.begin() + startPos + len, std::back_inserter(encText));

    const u32 pad = 512 - (len % 512);
    encText.resize(encText.size() + pad);

    std::vector<u8> plainText(encText.size());

    Rijndael::decrypt(psarcKey, encText.data(), plainText.data(), plainText.size());

    plainText.resize(len - startPos);

    return plainText;
}

static i32 tocBNum(const u32 blockSizeAlloc) {
    switch (blockSizeAlloc) {
        case 65536:
            return 2;
    }

    ASSERT(false);

    return {};
}

static void inflateTocEntry(Psarc::Info::TOCEntry &tocEntry, const u32 blockSizeAlloc, const u8 *psarcData,
                            const std::vector<u32> &zBlockSizeList) {
    if (tocEntry.length == 0)
        return;

    tocEntry.content.resize(tocEntry.length);

    const i32 zHeader = 0x78DA;
    u32 zChunkId = tocEntry.zIndexBegin;

    u64 inCur = 0;
    i32 outCur = 0;
    do {
        const u32 blockSize = zBlockSizeList[zChunkId];
        if (blockSize == 0) { // raw. full cluster used.
            memcpy(&tocEntry.content[outCur], &psarcData[tocEntry.offset + inCur], blockSizeAlloc);
            inCur += blockSizeAlloc;
            outCur += blockSizeAlloc;
        } else {
            const u16 num = u16_be(&psarcData[tocEntry.offset + inCur]);
            if (num == zHeader) {
                const i32 size = Inflate::inflate(&psarcData[tocEntry.offset + inCur], blockSize, &tocEntry.content[outCur], i32(tocEntry.length - outCur));
                inCur += blockSize;
                outCur += size;
            } else { // raw. used only for data(chunks) smaller than 64 kb
                memcpy(&tocEntry.content[outCur], &psarcData[tocEntry.offset + inCur], blockSize);
                inCur += blockSize;
                outCur += blockSize;
            }
        }
        zChunkId += 1;
    } while (outCur < tocEntry.length);
}

static void readManifest(std::vector<Psarc::Info::TOCEntry> &tocEnties, u32 blockSizeAlloc, const u8 *psarcData,
                         const std::vector<u32> &zBlockSizeList) {
    Psarc::Info::TOCEntry &tocEntry = tocEnties[0];

    ASSERT(tocEntry.name.empty());

    tocEntry.name = "NameBlock.bin";
    inflateTocEntry(tocEntry, blockSizeAlloc, psarcData, zBlockSizeList);

    { // read names of toc entries.
        i32 begin = 0;
        i32 tocIndex = 1;
        for (i32 i = 0; i < tocEntry.content.size(); ++i) {
            if (tocEntry.content[i] == '\n') {
                tocEnties[tocIndex++].name = std::string(reinterpret_cast<const char *>(&tocEntry.content[begin]), i - begin);
                begin = i + 1;
            }
        }
        tocEnties[tocIndex].name = std::string(reinterpret_cast<const char*>(&tocEntry.content[begin]), tocEntry.content.size() - begin);
    }
}

Psarc::Info Psarc::parse(const std::vector<u8> &psarcData) {
    Info psarcInfo;

    { // parse Header
        psarcInfo.header.magicNumber = u32_be(&psarcData[0]);
        ASSERT(psarcInfo.header.magicNumber == 1347633490_u32 && "Invalid Psarc content");
        psarcInfo.header.version = u32_be(&psarcData[4]);
        psarcInfo.header.compressMethod = u32_be(&psarcData[8]);
        psarcInfo.header.totalTocSize = u32_be(&psarcData[12]);
        psarcInfo.header.TOCEntrySize = u32_be(&psarcData[16]);
        psarcInfo.header.numFiles = u32_be(&psarcData[20]);
        psarcInfo.header.blockSizeAlloc = u32_be(&psarcData[24]);
        psarcInfo.header.archiveFlags = u32_be(&psarcData[28]);
    }

    { // parse TOC
        psarcInfo.tocRaw = decryptPsarc(psarcData, 32, psarcInfo.header.totalTocSize);
        for (u32 i = 0; i < psarcInfo.header.numFiles; ++i) {
            const u64 offset = i * 30;
            Info::TOCEntry tocEntry;
            memcpy(tocEntry.md5, &psarcInfo.tocRaw[offset], 16);
            tocEntry.zIndexBegin = u32_be(&psarcInfo.tocRaw[offset + 16]);
            tocEntry.length = u40_be(&psarcInfo.tocRaw[offset + 20]);
            tocEntry.offset = u40_be(&psarcInfo.tocRaw[offset + 25]);
            psarcInfo.tocEntries.push_back(tocEntry);
        }
    }

    std::vector<u32> zBlockSizeList;
    { // parse zBlockSizeList
        const i32 tocSize = psarcInfo.header.totalTocSize - 32;
        const i32 tocChunkSize = (int) (psarcInfo.header.numFiles * psarcInfo.header.TOCEntrySize);
        const i32 bNum = tocBNum(psarcInfo.header.blockSizeAlloc);
        const i32 zNum = (tocSize - tocChunkSize) / bNum;
        const u64 offset = psarcInfo.header.numFiles * 30;
        zBlockSizeList.resize(zNum);

        for (int i = 0; i < zNum; ++i) {
            switch (bNum) {
                case 2: // 64KB
                    zBlockSizeList[i] = u16_be(&psarcInfo.tocRaw[offset + i * 2]);
                    break;
                default:
                    ASSERT(false);
                    break;
            }
        }
    }

    { // decompress
        switch (psarcInfo.header.compressMethod) {
            case 2053925218:
                readManifest(psarcInfo.tocEntries, psarcInfo.header.blockSizeAlloc, &psarcData[0],
                             zBlockSizeList);
                break;
            default:
                ASSERT(false);
                break;
        }
    }

    { // inflate entries
        for (Info::TOCEntry &tocEntry: psarcInfo.tocEntries) {
            if (tocEntry.name == "NameBlock.bin")
                continue;

            inflateTocEntry(tocEntry, psarcInfo.header.blockSizeAlloc, &psarcData[0], zBlockSizeList);
        }
    }

    return psarcInfo;
}
