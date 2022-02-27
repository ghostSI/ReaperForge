#ifndef PSARC_H
#define PSARC_H

#include "typedefs.h"
#include <vector>

namespace Psarc {
    std::vector<u8> readPsarcData(const char *filepath);

    struct PsarcInfo {
        struct {
            u32 magicNumber;
            u32 version;
            u32 compressMethod;
            u32 totalTocSize;
            u32 TOCEntrySize;
            u32 numFiles;
            u32 BlockSizeAlloc;
            u32 archiveFlags;
        } header;

        struct {

        } image;

        struct {
           std::vector<u8> toc;
        } data;
    };

    PsarcInfo read(const std::vector<u8> &psarcData);
}

#endif // PSARC_H