
#include "type.h"

#include <vector>

namespace Settings {
    struct Resolution {
        int w, h;
    };

    extern DisplayMode displayMode;
    extern bool pauseAudio;
    extern u32 windowWidth;
    extern u32 windowHeight;

    extern std::vector<Resolution> supportedDisplayResolutions;
    extern i32 selectedSupportedDisplayResolution;

    void load();

    void save();
}