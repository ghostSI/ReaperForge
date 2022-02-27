
#include "settings.h"
#include "global.h"
#include "file.h"
#include "getopt.h"
#include "installer.h"

#include "SDL2/SDL_video.h"

#include <filesystem>


static std::filesystem::path settingsIniPath("settings.ini");

static const std::map<std::string, std::map<std::string, std::string>> defaultSettings = {
        {
                "Graphics",
                {
                        {"ResolutionWidth",  "800"},
                        {"ResolutionHeight", "600"},
                        {"Fullscreen",       "0"}

                }
        },
        {
                "Highway",
                {
                        {"Lyrics",           "1"}
                }
        },
        {
                "Instrument",
                {
                        {"BassStringColor0", "E05A01"},
                        {"BassStringColor1", "2381E9"},
                        {"BassStringColor2", "D2A20D"},
                        {"BassStringColor3", "D20000"},
                        {"BassStringColor4", "009B71"},
                        {"GuitarStringColor0", "940FB0"},
                        {"GuitarStringColor1", "1F9601"},
                        {"GuitarStringColor2", "E05A01"},
                        {"GuitarStringColor3", "2381E9"},
                        {"GuitarStringColor4", "D2A20D"},
                        {"GuitarStringColor5", "D20000"},
                        {"GuitarStringColor6", "009B71"}
                }
        },
        {
                "Library",
                {
                        {"Path",             "\"songs\""}
                }
        }

};
std::map<std::string, std::map<std::string, std::string>> commandLineSettings;
std::map<std::string, std::map<std::string, std::string>> gameSettings;

static void printUsage() {
    puts("Usage: ReaperForge.exe [OPTION]...\n"
         "Launches the game. Options will overwrite the in-game settings.\n"
         "\n"
         "Options:\n"
         "  -s            Path to settings.ini\n"
         "  -w            Resolution width\n"
         "  -h            Resolution height\n"
         "  -f            Fullscreen\n"
         "  -p            Path to song library directory that contains the .psarc files\n"
         "  -l            Show lyrics on highway\n"
         "  -0            Guitar string 0 color (e)\n"
         "  -1            Guitar string 1 color (b)\n"
         "  -2            Guitar string 2 color (G)\n"
         "  -3            Guitar string 3 color (D)\n"
         "  -4            Guitar string 4 color (A)\n"
         "  -5            Guitar string 5 color (E)\n"
         "  -6            Guitar string 6 color (B)\n"
         "  -g            Bass string 0 color (G)\n"
         "  -d            Bass string 1 color (D)\n"
         "  -a            Bass string 2 color (A)\n"
         "  -e            Bass string 3 color (E)\n"
         "  -b            Bass string 4 color (B)\n");

    fflush(stdout);
}

static bool parseCommandLineArgs(int argc, char *argv[]) {
    int c;
    while ((c = getopt(argc, argv, "s:w:h:f:p:l:0:1:2:3:4:5:6:g:d:a:e:b:")) != -1) {
        switch (c) {
            case 's':
                settingsIniPath = optarg;
                break;
            case 'w':
                commandLineSettings["Graphics"]["ResolutionWidth"] = optarg;
                break;
            case 'h':
                commandLineSettings["Graphics"]["ResolutionHeight"] = optarg;
                break;
            case 'f':
                commandLineSettings["Graphics"]["Fullscreen"] = optarg;
                break;
            case 'p':
                commandLineSettings["Library"]["Path"] = optarg;
                break;
            case 'l':
                commandLineSettings["Highway"]["Lyrics"] = optarg;
                break;
            case '0':
                commandLineSettings["Guitar"]["GuitarStringColor0"] = optarg;
                break;
            case '1':
                commandLineSettings["Guitar"]["GuitarStringColor1"] = optarg;
                break;
            case '2':
                commandLineSettings["Guitar"]["GuitarStringColor2"] = optarg;
                break;
            case '3':
                commandLineSettings["Guitar"]["GuitarStringColor3"] = optarg;
                break;
            case '4':
                commandLineSettings["Guitar"]["GuitarStringColor4"] = optarg;
                break;
            case '5':
                commandLineSettings["Guitar"]["GuitarStringColor5"] = optarg;
                break;
            case '6':
                commandLineSettings["Guitar"]["GuitarStringColor6"] = optarg;
                break;
            case 'g':
                commandLineSettings["Bass"]["BassStringColor6"] = optarg;
                break;
            case 'd':
                commandLineSettings["Bass"]["BassStringColor6"] = optarg;
                break;
            case 'a':
                commandLineSettings["Bass"]["BassStringColor6"] = optarg;
                break;
            case 'e':
                commandLineSettings["Bass"]["BassStringColor6"] = optarg;
                break;
            case 'b':
                commandLineSettings["Bass"]["BassStringColor6"] = optarg;
                break;
            default:
                printUsage();
                return false;
        }
    }

    return true;
}

bool Settings::init(int argc, char *argv[]) {

    if (!parseCommandLineArgs(argc, argv))
        return false;

    gameSettings = defaultSettings;

    if (Installer::isInstalled("."))
        gameSettings = File::loadIni("settings");

    File::saveIni(settingsIniPath.string().c_str(), gameSettings);

    return true;
}

std::string get(const std::string &section, const std::string &key) {
    if (commandLineSettings.contains(section) && commandLineSettings[section].contains(key))
        return commandLineSettings[section][key];

    if (gameSettings.contains(section) && gameSettings[section].contains(key))
        return gameSettings[section][key];

    ASSERT(false);

    return {};
}

void Settings::save() {
    File::saveIni(settingsIniPath.string().c_str(), gameSettings);
}

//static std::vector<Settings::Resolution> getSupportedDisplayResolutions() {
//    std::vector<Settings::Resolution> supportedDisplayResolutions;
//
//    const i32 displayCount = SDL_GetNumVideoDisplays();
//    for (i32 displayIdx = 0; displayIdx <= 1; ++displayIdx) {
//        const i32 modeCount = SDL_GetNumDisplayModes(displayIdx);
//        for (i32 modeIdx = 0; modeIdx <= modeCount; ++modeIdx) {
//            SDL_DisplayMode mode = {SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0};
//
//            if (SDL_GetDisplayMode(displayIdx, modeIdx, &mode) == 0) {
//                const size_t size = supportedDisplayResolutions.size();
//                if (size >= 1 && (supportedDisplayResolutions.at(size - 1).w == mode.w &&
//                                  supportedDisplayResolutions.at(size - 1).h))
//                    continue;
//
//                const Settings::Resolution resolution{
//                        .w = mode.w,
//                        .h = mode.h,
//                };
//                supportedDisplayResolutions.push_back(resolution);
//            }
//        }
//    }
//
//    return supportedDisplayResolutions;
//}