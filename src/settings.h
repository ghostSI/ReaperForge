#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

namespace Settings {

    bool init(int argc, char *argv[]);

    std::string get(const std::string &section, const std::string &key);

    void load();

    void save();
}

#endif // SETTINGS_H
