#pragma once
#include <vector>
#include "Core/Sprite.h"

// One frame of an animation: a sprite shown for `duration` seconds.
struct AnimationFrame {
    Sprite sprite;
    float duration = 0.1f;
};

// A timed sequence of sprites. Entities own one and the engine advances it each
// tick via update(dt). Looping animations cycle forever; non-looping ones stop
// on the last frame and report finished() (useful for transient effects).
class Animation {
public:
    Animation() = default;

    Animation& addFrame(const Sprite& sprite, float duration);
    void setLooping(bool looping) { looping_ = looping; }
    void reset();

    void update(float dt);

    const Sprite& current() const;   // safe even when empty (returns a default)
    bool finished() const { return finished_; }
    bool empty() const { return frames_.empty(); }

private:
    std::vector<AnimationFrame> frames_;
    bool   looping_ = true;
    float  timer_ = 0.0f;
    size_t index_ = 0;
    bool   finished_ = false;
};
