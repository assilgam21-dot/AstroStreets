#pragma once
#include <string>
#include <vector>
#include "Core/SpriteId.h"

// One sprite to draw, in world-pixel space (the camera transform is applied by
// the renderer). Position is the top-left corner.
struct SpriteDraw {
    SpriteId id = SpriteId::None;
    float x = 0, y = 0;
    float w = 0, h = 0;
    float r = 1, g = 1, b = 1, a = 1;   // tint
};

// One run of HUD text, in screen-pixel space (NOT affected by the camera).
struct TextDraw {
    int x = 0, y = 0;
    std::string text;
    float r = 1, g = 1, b = 1;
};

// The camera as seen by the renderer: the world-pixel point shown at the center
// of the screen, and a zoom factor. The full camera logic lives in Game/Camera.
struct CameraState {
    float x = 0, y = 0;
    float zoom = 1.0f;
};

// A point light in world-pixel space. radius is in world pixels; intensity
// scales the contribution. The renderer adds these on top of the ambient term.
struct LightDraw {
    float x = 0, y = 0;
    float r = 1, g = 1, b = 1;
    float radius = 0;
    float intensity = 1;
};

// A complete frame produced by the simulation thread and consumed by the render
// thread — the snapshot exchanged through SnapshotBuffer.
struct RenderFrame {
    std::vector<SpriteDraw> sprites;
    std::vector<TextDraw>   texts;
    std::vector<LightDraw>  lights;
    CameraState             camera;
    float ambientR = 0.40f, ambientG = 0.40f, ambientB = 0.46f;

    void clear() {
        sprites.clear();
        texts.clear();
        lights.clear();
    }
};
