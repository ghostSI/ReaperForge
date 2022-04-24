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
        anchor,
        ebeat,
        chordBox,
        chordBoxConsecutive,
        chordBoxArpeggio,
        phrasesScreen,
        handShapeAnchor,
        fretGold,
        fretSilver,
        fretBronze,
        string,
        noteStand,
        noteStandZero,
    };

    void init();

    GLuint useShader(Shader::Stem shaderStem);
}

#endif // SHADER_H
