#ifndef DEVUTIL_H
#define DEVUTIL_H


#include "stdio.h"
#include "typedefs.h"

inline void dumpDataToFile(const u8 *data, u64 len, u32 columnWidth, bool hex) {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
    FILE *file = fopen("dump.txt", "w");
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32

    for (int i = 0; i < len; ++i) {
        if (hex)
            fprintf(file, "%02X ", data[i]);
        else
            fprintf(file, "%u,", data[i]);

        if ((i + 1) % columnWidth == 0)
            fprintf(file, "\n");
        else
            fprintf(file, " ");
    }

    fclose(file);
}

#endif // DEVUTIL_H