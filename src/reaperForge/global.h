#ifndef GLOBAL_H
#define GLOBAL_H

#include "type.h"
#include "helper.h"

#include <vector>
#include <string>

enum struct DisplayMode : u8;
enum struct InputDebugMode : u16;
enum struct RenderOptions : u32;
namespace Biom { enum struct Type : u8; }
namespace Weapon { enum struct Type : i32; }
namespace World { enum struct Type : i32; }
struct SDL_Window;
typedef struct _SDL_GameController SDL_GameController;

namespace Const
{
inline constexpr i32 randomIntMax = 65535;
inline constexpr i16 controllerAxisDeadZone = 3000;
inline constexpr i16 controllerTriggerDeadZone = -30000;
}

namespace Global
{
extern SDL_Window* window;
extern SDL_GameController* gameController;
extern bool appQuit;
extern KeyInput inputFullscreen;
extern KeyInput inputPause;

extern u32 windowWidth;
extern u32 windowHeight;
extern DisplayMode displayMode;
extern bool pauseAudio;

extern f32 cameraMidX;
extern f32 cameraMidY;

extern Biom::Type cameraBiom;
extern Biom::Type defaultBiom;

extern f32 frameDelta;
extern f32 time;

extern char guyName[256];

extern bool inputUseController;
extern i32 inputCursorPosX;
extern i32 inputCursorPosY;

extern RenderOptions renderOptions;

extern GLuint vao; // default vao
extern GLuint vbo; // default vbo
extern GLuint fbo; // default fbo is 0 if postprocessor is off
extern GLuint fboRtt; // framebuffer for rendering to texture (make sure to set framebuffer to 0 after using it)
};

#endif // GLOBAL_H
