#pragma once
#include <mutex>
#include "Core/RenderFrame.h"

// Hands a finished frame from the simulation thread to the render thread.
//
// The sim thread calls publish() with its latest RenderFrame; the render thread
// calls acquire() to copy out the newest one. If the renderer is faster it
// redraws the last frame (acquire returns false); if the sim is faster,
// intermediate frames are dropped — exactly what you want for display.
class SnapshotBuffer {
public:
    void publish(const RenderFrame& frame) {
        std::lock_guard<std::mutex> lk(mutex_);
        frame_ = frame;
        dirty_ = true;
    }

    bool acquire(RenderFrame& out) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (!dirty_) return false;
        out = frame_;
        dirty_ = false;
        return true;
    }

private:
    std::mutex mutex_;
    RenderFrame frame_;
    bool dirty_ = false;
};
