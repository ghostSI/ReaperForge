#ifndef SHADER_H
#define SHADER_H

#include "typedefs.h"

namespace Shader {
    enum struct Stem {
        defaultScreen,
        defaultWorld,
        ground,
        fontScreen,
        fontWorld,
        anchorWorld,
        chordBoxWorld,
        phrasesScreen,
        handShapeWorld,
    };

    void init();

    GLuint useShader(Shader::Stem shaderStem);
}

#endif // SHADER_H
