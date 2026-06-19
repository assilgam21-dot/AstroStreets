#include "Core/Animation.h"

Animation& Animation::addFrame(const Sprite& sprite, float duration) {
    frames_.push_back(AnimationFrame{ sprite, duration });
    return *this;
}

void Animation::reset() {
    timer_ = 0.0f;
    index_ = 0;
    finished_ = false;
}

void Animation::update(float dt) {
    if (frames_.size() < 2 || finished_) return;

    timer_ += dt;
    while (timer_ >= frames_[index_].duration) {
        timer_ -= frames_[index_].duration;
        if (index_ + 1 < frames_.size()) {
            ++index_;
        } else if (looping_) {
            index_ = 0;
        } else {
            finished_ = true;
            break;
        }
    }
}

const Sprite& Animation::current() const {
    static const Sprite kDefault{ SpriteId::None };
    if (frames_.empty()) return kDefault;
    return frames_[index_].sprite;
}
