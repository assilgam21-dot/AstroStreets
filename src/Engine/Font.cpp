#include "Engine/Font.h"
#include "Engine/font8x8_basic.h"   // char font8x8_basic[128][8] (public domain)

#include <glad/glad.h>
#include <vector>

unsigned int CreateFontAtlas() {
    // 16 glyphs across, 8 down -> 128x64 single-channel atlas.
    const int AW = 16 * 8, AH = 8 * 8;
    std::vector<unsigned char> pixels(size_t(AW) * size_t(AH), 0);

    for (int c = 0; c < 128; ++c) {
        int gx0 = (c % 16) * 8, gy0 = (c / 16) * 8;
        for (int row = 0; row < 8; ++row) {
            unsigned char bits = (unsigned char)font8x8_basic[c][row];
            for (int col = 0; col < 8; ++col) {
                if (bits & (1 << col))  // LSB = leftmost pixel
                    pixels[size_t(gy0 + row) * AW + (gx0 + col)] = 255;
            }
        }
    }

    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, AW, AH, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}
