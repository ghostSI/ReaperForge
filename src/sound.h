#ifndef SOUND_H
#define SOUND_H

#include "typedefs.h"

namespace Sound {
    enum struct Effect {
        menuHover,
        menuSelect,
    };

    void init();

    void play(Sound::Effect type, i32 volume = 64);

    void setPauseAudio(bool pauseAudio);
};

#endif // SOUND_H
