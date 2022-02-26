#ifndef FONT_H
#define FONT_H

#include "type.h"

namespace Font {
    using Handle = i32;

    enum struct Alignment : i32 {
        left,
        center,
        right
    };

    enum struct Type : i32 {
        font,
        fontEvil,
        COUNT
    };

    struct Info {
        const char *text = nullptr;
        Font::Handle fontHandle = 0;
        f32 posX = 0.0_f32;
        f32 posY = 0.0_f32;
        Font::Alignment alignment = Font::Alignment::left;
        Space space = Space::worldSpace;
        Font::Type type = Font::Type::font;
        f32 autoDelete = F32::inf;
    };

    void init();

    Font::Handle print(const Font::Info &fontInfo);

    void remove(Font::Handle id);

    void tick();

    void render();
}

#endif // FONT_H
