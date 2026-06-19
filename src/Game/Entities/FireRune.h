#pragma once
#include "Game/Entities/AreaTrigger.h"

// A concrete area trigger: a flickering rune that burns units standing on it
// (damage-over-time). Shows how to specialize AreaTrigger and how an animation
// drives a multi-tile entity's appearance.
class FireRune : public AreaTrigger {
public:
    FireRune(World* world, int width, int height);

    void OnUpdate(float dt) override;
    void OnUnitEnter(Unit* unit) override;
    void OnUnitExit(Unit* unit) override;

private:
    float tick_ = 0.0f;
    int   damagePerSecond_ = 1;
};
