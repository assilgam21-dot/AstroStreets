#pragma once
#include <cstdint>
#include <unordered_set>
#include "Game/Entities/Entity.h"

class Unit;
struct RenderFrame;

// A rectangular trigger volume anchored at (x, y) spanning width x height tiles.
// Each tick it figures out which units are inside and fires enter/exit/stay
// hooks. Subclasses implement the actual effect (damage, teleport, script...).
class AreaTrigger : public Entity {
public:
    AreaTrigger(World* world, int width, int height);

    EntityType GetType() const override { return EntityType::AreaTrigger; }

    int  GetWidth() const { return width_; }
    int  GetHeight() const { return height_; }
    bool Contains(int px, int py) const;

    void OnUpdate(float dt) override;            // detect membership, fire hooks
    void Render(RenderFrame& frame) const override; // tile the rectangle

    virtual void OnUnitEnter(Unit* unit) { (void)unit; }
    virtual void OnUnitExit(Unit* unit)  { (void)unit; }
    virtual void OnUnitStay(Unit* unit, float dt) { (void)unit; (void)dt; }

protected:
    int width_, height_;
    std::unordered_set<uint32_t> inside_;   // ids of units currently inside
};
