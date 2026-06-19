#include "Game/Entities/AreaTrigger.h"
#include "Game/Entities/Unit.h"
#include "Game/World.h"
#include "Core/RenderFrame.h"
#include "Core/Constants.h"

AreaTrigger::AreaTrigger(World* world, int width, int height)
    : Entity(world), width_(width), height_(height) {
    SetName("AreaTrigger");
}

bool AreaTrigger::Contains(int px, int py) const {
    return px >= x_ && px < x_ + width_ &&
           py >= y_ && py < y_ + height_;
}

void AreaTrigger::OnUpdate(float dt) {
    std::unordered_set<uint32_t> nowInside;

    for (Unit* unit : GetWorld()->GetUnits()) {
        if (!unit->IsAlive() || !Contains(unit->GetX(), unit->GetY())) continue;
        nowInside.insert(unit->GetId());
        if (inside_.find(unit->GetId()) == inside_.end())
            OnUnitEnter(unit);
        OnUnitStay(unit, dt);
    }

    // anyone who was inside but isn't now has left
    for (Unit* unit : GetWorld()->GetUnits()) {
        if (inside_.count(unit->GetId()) && !nowInside.count(unit->GetId()))
            OnUnitExit(unit);
    }

    inside_.swap(nowInside);
}

void AreaTrigger::Render(RenderFrame& frame) const {
    const Sprite& s = animation_.empty() ? sprite_ : animation_.current();
    if (s.id == SpriteId::None) return;
    for (int yy = y_; yy < y_ + height_; ++yy)
        for (int xx = x_; xx < x_ + width_; ++xx)
            frame.sprites.push_back(SpriteDraw{
                s.id,
                float(xx * TILE_SIZE), float(yy * TILE_SIZE),
                float(TILE_SIZE), float(TILE_SIZE),
                s.r, s.g, s.b, s.a });
}
