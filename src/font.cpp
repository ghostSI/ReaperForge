#include "font.h"

#include "sprite.h"
#include "global.h"

#include <vector>
#include <string.h>

static const i32 charWidth = 12;
static const i32 charHeight = 18;

struct Text {
    Space space;
    Font::Handle fontHandle;
    f32 textPosX;
    f32 textPosY;
    i32 letters;
    f32 autoDelete;
    std::vector<u32> textBitmap;
};

static std::vector<Text> texts;

static i32 indexOfHandle(Font::Handle fontHandle) {
    ASSERT(fontHandle >= 1);

    for (i32 i = 0; i < i32(texts.size()); ++i)
        if (fontHandle == texts.at(i).fontHandle)
            return i;

    return -1;
}

static Texture::Type textureType(Font::Type fontType) {
    return Texture::Type(to_underlying(Texture::Type::font) + to_underlying(fontType));
}

static const u32 *texture(Font::Type fontType) {
    return reinterpret_cast<const u32 *>(Texture::texture(textureType(fontType)));
}

static const i32 textureWidth(Font::Type fontType) {
    return Texture::width(textureType(fontType));
}

void Font::init() {
    texts.clear();
}

Font::Handle Font::print(const Info &fontInfo) {
    Text text;
    text.space = fontInfo.space;
    text.autoDelete = fontInfo.autoDelete;

    static i32 fontHandle;
    i32 foundIndex = -1;
    if (fontInfo.fontHandle == 0) {
        text.fontHandle = ++fontHandle;
    } else {
        foundIndex = indexOfHandle(fontInfo.fontHandle);
        if (foundIndex >= 0)
            text.fontHandle = texts[foundIndex].fontHandle;
        else
            text.fontHandle = ++fontHandle;
    }

    text.letters = i32(strlen(fontInfo.text));

    switch (fontInfo.alignment) {
        case Font::Alignment::left:
            text.textPosX = fontInfo.posX + 0.5_f32 * f32(text.letters * charWidth);
            break;
        case Font::Alignment::center:
            text.textPosX = fontInfo.posX;
            break;
        case Font::Alignment::right:
            text.textPosX = fontInfo.posX - 0.5_f32 * f32(text.letters * charWidth);
            break;
    }
    text.textPosY = fontInfo.posY;

    text.textBitmap.resize(i64(text.letters) * charWidth * charHeight);

    const i32 rowOffset = (text.letters - 1) * charWidth;

    const i32 textureWidth_ = textureWidth(fontInfo.type);

    for (int c = 0; c < text.letters; ++c) {
        char letter = fontInfo.text[c];

        if (letter < ' ' || letter > '~')
            letter = '~' + 1;

        i32 offsetY = ((letter - ' ') / 16) * charHeight;
        i32 offsetX = (letter % 16) * charWidth;

        i32 i = charWidth * c;
        for (i32 y = 0; y < charHeight; ++y) {
            for (i32 x = 0; x < charWidth; ++x) {
                text.textBitmap[i] = texture(fontInfo.type)[(y + offsetY) * textureWidth_ + (x + offsetX)];

                ++i;

                if (i % charWidth == 0)
                    i += rowOffset;
            }
        }
    }
    if (fontInfo.fontHandle == 0 || foundIndex == -1)
        texts.push_back(text); // add new text
    else
        texts[foundIndex] = text; // update existing text

    return text.fontHandle;
}

void Font::remove(Font::Handle fontHandle) {
    if (fontHandle <= 0)
        return;

    const i32 index = indexOfHandle(fontHandle);
    if (index >= 0)
        texts.erase(texts.begin() + index);
}

void Font::tick() {
    for (i32 i = i32(texts.size()) - 1; i >= 0; --i) {
        if (texts[i].autoDelete != F32::inf) {
            texts[i].autoDelete -= Global::frameDelta;
            if (texts[i].autoDelete < 0.0_f32) {
                remove(texts[i].fontHandle);
            }
        }
    }
}

void Font::render() {
    for (const Text &text: texts) {
        Sprite::Info spriteInfo{
                .shaderStem = text.space == Space::worldSpace ? Shader::Stem::defaultWorld
                                                              : Shader::Stem::fontScreen,
                .zOrder = text.space == Space::worldSpace ? ZOrder::fontWorld : ZOrder::fontUi,
                .posX = text.textPosX,
                .posY = text.textPosY,
                .space = text.space,
                .skipTextureCache = true,
                .rawTexture = reinterpret_cast<const u8 *>(text.textBitmap.data()),
                .rawTextureWidth = text.letters * charWidth,
                .rawTextureHeight = charHeight,
        };
        Sprite::render(spriteInfo);
    }
}
