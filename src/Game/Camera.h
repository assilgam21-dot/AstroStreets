#pragma once
#include "Core/RenderFrame.h"   // CameraState

// The camera. Lives on the simulation thread; each tick it is given the focus
// target (usually the player) and produces a CameraState (world-center + zoom)
// for the renderer. Supports three modes plus an animated focus:
//
//   Follow  - smoothly tracks the target at the follow zoom.
//   Locked  - holds its current position/zoom, ignoring the target.
//   Focus   - eases to a chosen point/zoom, holds, then returns (to Follow) or
//             stays (Locked). Used for "look at this area" moments.
//
// Zoom is adjustable at any time and clamped to a sane range.
enum class CameraMode { Follow, Locked, Focus };

class Camera {
public:
    void Update(float dt, float targetX, float targetY);  // targetX/Y = focus point (world px)
    CameraState State() const { return { x_, y_, zoom_ }; }
    CameraMode  Mode() const { return mode_; }

    void SetFollow();
    void SetLocked();
    void FocusOn(float x, float y, float zoom, float easeIn, float hold, bool returnAfter);

    void SetZoom(float zoom);
    void AddZoom(float delta);
    float GetZoom() const { return zoom_; }

    void SnapTo(float x, float y) { x_ = x; y_ = y; }     // teleport (no easing)

private:
    static float Clampf(float v, float lo, float hi);

    CameraMode mode_ = CameraMode::Follow;
    float x_ = 0.0f, y_ = 0.0f, zoom_ = 1.0f;
    float followZoom_ = 1.0f;
    float followLerp_ = 6.0f;     // higher = snappier follow
    bool  initialized_ = false;

    // focus animation
    int   focusPhase_ = 0;        // 0 ease-in, 1 hold
    float focusFromX_ = 0, focusFromY_ = 0, focusFromZoom_ = 1;
    float focusToX_ = 0, focusToY_ = 0, focusToZoom_ = 1;
    float focusT_ = 0, focusEaseIn_ = 1, focusHold_ = 1;
    bool  focusReturn_ = true;
};
