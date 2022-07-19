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
        dotInlay,
        groundFret,
        fontScreen,
        fontWorld,
        anchor,
        ebeat,
        chordBox,
        chordBoxConsecutive,
        chordBoxFretMute,
        chordBoxPalmMute,
        chordBoxArpeggio,
        phrasesScreen,
        handShapeAnchor,
        fretGold,
        fretSilver,
        fretBronze,
        string,
        sustain,
        noteStand,
        noteStandZero,
        ui,
        COUNT
    };

    void init();

    GLuint useShader(Shader::Stem shaderStem);
}

#endif // SHADER_H
