#pragma once
#include "Core/SpriteId.h"

// A reference to an image asset plus an optional tint (multiplied with the
// texture, 0..1). Tinting lets several entities share one texture in different
// colors (e.g. a grayscale grunt drawn red or orange).
struct Sprite {
    SpriteId id = SpriteId::None;
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;

    Sprite() = default;
    Sprite(SpriteId id_, float r_ = 1.0f, float g_ = 1.0f, float b_ = 1.0f, float a_ = 1.0f)
        : id(id_), r(r_), g(g_), b(b_), a(a_) {}
};
