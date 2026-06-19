#pragma once
#include <string>
#include <vector>
#include "Engine/SpriteManager.h"

struct RenderFrame;

// Draws a RenderFrame each frame in two passes:
//   1) sprites in world space, transformed by the frame's camera (zoom + pan),
//      alpha-blended, batched into contiguous same-texture runs;
//   2) HUD text in screen space (no camera), from the bitmap font atlas.
//
// THREADING: render thread only — every method needs a current GL context.
class Renderer {
public:
    bool init(const std::string& assetsDir);
    void shutdown();
    void draw(const RenderFrame& frame, int viewportW, int viewportH);

private:
    void drawSprites(const RenderFrame& frame, int w, int h);
    void drawText(const RenderFrame& frame, int w, int h);

    SpriteManager sprites_;

    unsigned int spriteProg_ = 0;   // RGBA texture * tint
    unsigned int textProg_ = 0;     // R8 font as coverage
    unsigned int fontTex_ = 0;
    unsigned int vao_ = 0, vbo_ = 0;

    std::vector<float> scratch_;    // reused vertex buffer
};
