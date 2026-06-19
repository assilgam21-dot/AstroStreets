#include "Game/Entities/FireRune.h"
#include "Game/Entities/Unit.h"
#include "Game/Entities/DamageInfo.h"
#include "Game/World.h"

FireRune::FireRune(World* world, int width, int height)
    : AreaTrigger(world, width, height) {
    SetName("Fire Rune");
    // cycle the three rune frames (image-based animation)
    GetAnimation()
        .addFrame(Sprite(SpriteId::FireRune0), 0.18f)
        .addFrame(Sprite(SpriteId::FireRune1), 0.16f)
        .addFrame(Sprite(SpriteId::FireRune2), 0.20f);
}

void FireRune::OnUpdate(float dt) {
    AreaTrigger::OnUpdate(dt);   // membership detection + enter/exit hooks

    tick_ += dt;
    if (tick_ < 1.0f) return;
    tick_ -= 1.0f;

    // burn everyone standing on the rune
    for (Unit* unit : GetWorld()->GetUnits()) {
        if (unit->IsAlive() && Contains(unit->GetX(), unit->GetY())) {
            DamageInfo dmg(damagePerSecond_, DamageSchool::Fire);
            unit->TakeDamage(dmg);
        }
    }
}

void FireRune::OnUnitEnter(Unit* unit) {
    GetWorld()->AddMessage(unit->GetName() + " steps into the fire rune!");
}

void FireRune::OnUnitExit(Unit* unit) {
    GetWorld()->AddMessage(unit->GetName() + " leaves the fire rune.");
}
