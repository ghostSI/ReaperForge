#ifndef FILE_H
#define FILE_H

#include "typedefs.h"

#include <string>
#include <vector>

namespace File {
    struct IniContent {
        struct Entry {
            std::string key;
            std::string value;
        };
        std::string name;
        std::vector<Entry> entry;
    };

    bool exists(const char *filepath);

    std::vector<u8> load(const char *filepath, const char *mode);

    void load(const char *filepath, std::string &buffer);

    void save(const char *filepath, const char *content, size_t len);

    std::vector<u8> loadPng(const char *filepath, i32 &width, i32 &heigth, bool convertRGBA = true);

    std::vector<u8> loadPng(const u8 *imageData, u32 imageSize, i32 &width, i32 &heigth, bool convertRGBA = true);

    std::vector<IniContent> loadIni(const char *filepath);
}

#endif // FILE_H
