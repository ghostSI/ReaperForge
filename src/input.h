#ifndef INPUT_H
#define INPUT_H

union SDL_Event;

namespace Input {
    void prePollEvent();

    void pollEvent(SDL_Event &event);

    void postPollEvent();

    void proccessInputEvents();
}


#endif // INPUT_H
