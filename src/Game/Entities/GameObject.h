#pragma once
#include "Game/Entities/Entity.h"

class Unit;

// A non-living world object: chest, door, lever, decoration. It is not a Unit
// (no health/combat) but can be animated and interacted with via OnUse.
class GameObject : public Entity {
public:
    explicit GameObject(World* world);

    EntityType GetType() const override { return EntityType::GameObject; }

    bool IsUsable() const { return usable_; }
    void SetUsable(bool usable) { usable_ = usable; }

    virtual void OnUse(Unit* user) { (void)user; }

protected:
    bool usable_ = true;
};
