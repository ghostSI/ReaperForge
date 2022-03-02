#include "file.h"

#include "imageload.h"

#include <stdio.h>
#include <string>
#include <fstream>
#include <filesystem>

bool File::exists(const char *filepath) {
    return std::filesystem::exists(std::filesystem::path(filepath));
}

std::vector<u8> File::load(const char *filepath, const char *mode) {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
    FILE *file = fopen(filepath, mode);
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32

    fseek(file, 0, SEEK_END);
    const i32 lSize = ftell(file);
    rewind(file);

    std::vector<u8> fileData(lSize);
    fread(fileData.data(), lSize, 1, file);

    fclose(file);

    return fileData;
}

void File::load(const char *filepath, std::string &buffer) {
    std::ifstream file(filepath);

    std::stringstream ss;

    ss << file.rdbuf();

    buffer = ss.str();
}

void File::save(const char *filepath, const char *content, size_t len) {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
    FILE *file = fopen(filepath, "w");
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32

    fwrite(content, len, 1, file);

    fclose(file);
}

std::vector<u8> File::loadPng(const char *filepath, i32 &width, i32 &heigth, bool convertRGBA) {
    const std::vector<u8> fileData = File::load(filepath, "rb");

    return loadPng(fileData.data(), fileData.size(), width, heigth, convertRGBA);
}

GLuint File::loadDds(const char *filepath) {
    const std::vector<u8> fileData = File::load(filepath, "rb");

    return loadDDS(fileData.data(), fileData.size());
}

std::vector<u8> File::loadPng(const u8 *imageData, u32 imageSize, i32 &width, i32 &heigth, bool convertRGBA) {
    std::vector<u8> color;

    // conversion needed for emscripten
    unsigned long convWidth = width;
    unsigned long convHeight = heigth;
    decodePNG(color, convWidth, convHeight, imageData, imageSize, convertRGBA);
    width = convWidth;
    heigth = convHeight;

    return color;
}

std::map<std::string, std::map<std::string, std::string>> File::loadIni(const char *filepath) {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
    FILE *file = fopen(filepath, "r");
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32

    std::map<std::string, std::map<std::string, std::string>> iniContent;

    char buf[1024];
    while (fgets(buf, 1024, file) != nullptr) {
        std::string line = buf;

        if (line[0] == ';' || line[0] == '\n' || line[0] == '\r')
            continue;

        while (line.at(line.size() - 1) == '\n' || line.at(line.size() - 1) == '\r')
            line.pop_back();

        std::string name;
        if (line[0] == '[') {
            name = line.substr(1, line.size() - 2);
        } else {
            for (u64 i = 0; i < line.size(); ++i) {
                if (line[i] == '=') {
                    const std::string key = line.substr(0, i);
                    const std::string value = line.substr(i + 1, line.size() - i - 1);
                    iniContent[name] = {{key, value}};
                    break;
                }
            }
        }
    }

    fclose(file);

    return iniContent;
}

void File::saveIni(const char *filepath, const std::map<std::string, std::map<std::string, std::string>> &iniContent) {
#ifdef _WIN32
#pragma warning( disable: 4996 ) // ignore msvc unsafe warning
#endif // _WIN32
    FILE *file = fopen(filepath, "w");
#ifdef _WIN32
#pragma warning( default: 4996 )
#endif // _WIN32

    for (const auto& [section, keyValue] : iniContent)
    {
        fprintf(file, "[%s]\n", section.c_str());

        for (const auto& [key, value] : keyValue)
            fprintf(file, "%s=%s\n", key.c_str(), value.c_str());

        fputc('\n', file);
    }

    fclose(file);
}
