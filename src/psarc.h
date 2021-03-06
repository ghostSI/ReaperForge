#ifndef PSARC_H
#define PSARC_H

#include "typedefs.h"
#include <vector>

namespace Psarc {
    std::vector<u8> readPsarcData(const char *filepath);

    struct Info {
        struct {
            u32 magicNumber;
            u32 version;
            u32 compressMethod;
            u32 totalTocSize;
            u32 TOCEntrySize;
            u32 numFiles;
            u32 blockSizeAlloc;
            u32 archiveFlags;
        } header;

        std::vector<u8> tocRaw;
        struct TOCEntry{
            std::string name;
            u8 md5[16];
            u32 zIndexBegin;
            u64 length;
            u64 offset;
            std::vector<u8> content;
        };
        std::vector<TOCEntry> tocEntries;
    };

    Info parse(const std::vector<u8> &psarcData);

    //void loadOgg(const Info& psarcInfo, bool preview);
}

#endif // PSARC_H