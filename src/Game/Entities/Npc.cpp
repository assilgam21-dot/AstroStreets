#include "Game/Entities/Npc.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/DamageInfo.h"
#include "Game/World.h"

#include <cstdlib>

namespace {
int sign(int v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }
const float STEP_DELAY = 0.25f;
const float BITE_DELAY = 0.8f;
}

Npc::Npc(World* world) : Unit(world) {
    SetName("Grunt");
    SetMaxHealth(6);
    SetHealth(6);
    SetSpeed(4.0f);
    SetAttackRange(1);
    SetAttackPower(2);

    // a subtle two-frame idle pulse, tinting the shared grunt texture red
    GetAnimation()
        .addFrame(Sprite(SpriteId::Grunt, 1.0f, 0.45f, 0.45f), 0.5f)
        .addFrame(Sprite(SpriteId::Grunt, 0.85f, 0.30f, 0.30f), 0.5f);
}

void Npc::TryMove(int dx, int dy) {
    int nx = x_ + dx, ny = y_ + dy;
    if (!GetWorld()->IsWall(nx, ny)) SetPosition(nx, ny);
}

void Npc::OnUpdate(float dt) {
    Player* player = GetWorld()->GetPlayer();
    if (!player || !player->IsAlive()) return;

    stepCooldown_ -= dt;
    biteCooldown_ -= dt;

    int dist = DistanceTo(player);

    // bite on contact
    if (dist <= GetAttackRange() && biteCooldown_ <= 0.0f) {
        DamageInfo dmg(GetAttackPower(), DamageSchool::Physical);
        DealDamage(player, dmg);
        biteCooldown_ = BITE_DELAY;
    }

    if (stepCooldown_ > 0.0f) return;
    stepCooldown_ = STEP_DELAY;

    if (dist <= sightRange_) {
        // chase: step along the dominant axis
        int ddx = player->GetX() - x_, ddy = player->GetY() - y_;
        if (std::abs(ddx) > std::abs(ddy)) TryMove(sign(ddx), 0);
        else                               TryMove(0, sign(ddy));
    } else {
        // wander
        switch (std::rand() % 4) {
            case 0: TryMove( 1, 0); break;
            case 1: TryMove(-1, 0); break;
            case 2: TryMove( 0, 1); break;
            default: TryMove(0, -1); break;
        }
    }
}

void Npc::OnDamageTaken(Unit* attacker, DamageInfo* damage) {
    (void)attacker;
    GetWorld()->AddMessage(GetName() + " takes " + std::to_string(damage->GetAmount()) + " damage");
}

void Npc::OnDeath(Unit* killer) {
    (void)killer;
    GetWorld()->AddMessage(GetName() + " dies.");
}
