#ifndef SHADER_H
#define SHADER_H

#include "typedefs.h"

namespace Shader {
    enum struct Stem {
        defaultScreen,
        defaultWorld,
        detectedChord,
        detectedFret,
        detectedNote,
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
        ui,
    };

    void init();

    GLuint useShader(Shader::Stem shaderStem);
}

#endif // SHADER_H
