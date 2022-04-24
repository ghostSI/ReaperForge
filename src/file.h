#ifndef FILE_H
#define FILE_H

#include "typedefs.h"

#include <string>
#include <vector>
#include <map>

namespace File {
    bool exists(const char *filepath);

    std::vector<u8> load(const char *filepath, const char *mode);

    void load(const char *filepath, std::string &buffer);

    void save(const char *filepath, const char *content, size_t len);

    GLuint loadDds(const char *filepath);

    std::map<std::string, std::map<std::string, std::string>> loadIni(const char *filepath);
    void saveIni(const char *filepath, const std::map<std::string, std::map<std::string, std::string>>& iniContent);
}

#endif // FILE_H
