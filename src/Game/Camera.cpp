#include "Game/Camera.h"

#include <algorithm>
#include <cmath>

namespace {
float smoothstep(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
float lerp(float a, float b, float t) { return a + (b - a) * t; }
}

float Camera::Clampf(float v, float lo, float hi) { return std::clamp(v, lo, hi); }

void Camera::SetFollow() { mode_ = CameraMode::Follow; }
void Camera::SetLocked() { mode_ = CameraMode::Locked; }

void Camera::SetZoom(float zoom) {
    followZoom_ = Clampf(zoom, 0.4f, 4.0f);
    if (mode_ != CameraMode::Focus) zoom_ = followZoom_;
}
void Camera::AddZoom(float delta) { SetZoom(followZoom_ + delta); }

void Camera::FocusOn(float x, float y, float zoom, float easeIn, float hold, bool returnAfter) {
    mode_ = CameraMode::Focus;
    focusPhase_ = 0;
    focusFromX_ = x_; focusFromY_ = y_; focusFromZoom_ = zoom_;
    focusToX_ = x; focusToY_ = y; focusToZoom_ = Clampf(zoom, 0.4f, 4.0f);
    focusT_ = 0.0f;
    focusEaseIn_ = std::max(0.0001f, easeIn);
    focusHold_ = std::max(0.0f, hold);
    focusReturn_ = returnAfter;
}

void Camera::Update(float dt, float targetX, float targetY) {
    if (!initialized_) {           // start centered on the target, no swoop-in
        x_ = targetX; y_ = targetY; zoom_ = followZoom_;
        initialized_ = true;
    }

    switch (mode_) {
        case CameraMode::Follow: {
            float t = std::min(1.0f, followLerp_ * dt);   // exponential-ish smoothing
            x_ = lerp(x_, targetX, t);
            y_ = lerp(y_, targetY, t);
            zoom_ = lerp(zoom_, followZoom_, t);
            break;
        }
        case CameraMode::Locked:
            break;   // hold

        case CameraMode::Focus: {
            focusT_ += dt;
            if (focusPhase_ == 0) {
                float a = smoothstep(focusT_ / focusEaseIn_);
                x_ = lerp(focusFromX_, focusToX_, a);
                y_ = lerp(focusFromY_, focusToY_, a);
                zoom_ = lerp(focusFromZoom_, focusToZoom_, a);
                if (focusT_ >= focusEaseIn_) { focusPhase_ = 1; focusT_ = 0.0f; }
            } else {
                x_ = focusToX_; y_ = focusToY_; zoom_ = focusToZoom_;
                if (focusT_ >= focusHold_)
                    mode_ = focusReturn_ ? CameraMode::Follow : CameraMode::Locked;
            }
            break;
        }
    }
}
