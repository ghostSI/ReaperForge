#ifndef UI_H
#define UI_H

union SDL_Event;

namespace Ui {
    void init();

    void tick();

    void render();

    void handleInputBegin();

    void handleInput(SDL_Event &event);

    void handleInputEnd();
}

#endif // UI_H
