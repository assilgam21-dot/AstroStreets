#include "Game/Entities/Player.h"
#include "Game/Entities/DamageInfo.h"
#include "Game/World.h"
#include "Core/Input.h"

#include <vector>

Player::Player(World* world) : Unit(world) {
    SetName("Player");
    SetMaxHealth(20);
    SetHealth(20);
    SetSpeed(8.0f);        // tiles/sec when holding a direction
    SetAttackRange(1);
    SetAttackPower(5);
    sprite_ = Sprite(SpriteId::Player);
}

void Player::OnSpawn() {
    GetWorld()->AddMessage("WASD/arrows: move   Space: attack   Esc: quit");
}

void Player::TryMove(int dx, int dy) {
    int nx = x_ + dx, ny = y_ + dy;
    if (!GetWorld()->IsWall(nx, ny)) SetPosition(nx, ny);
}

void Player::OnUpdate(float dt) {
    moveCooldown_ -= dt;
    if (moveCooldown_ > 0.0f) return;

    Input& in = GetWorld()->GetInput();
    int dx = 0, dy = 0;
    if (in.keyDown("a") || in.keyDown("left"))  dx = -1;
    if (in.keyDown("d") || in.keyDown("right")) dx =  1;
    if (in.keyDown("w") || in.keyDown("up"))    dy = -1;
    if (in.keyDown("s") || in.keyDown("down"))  dy =  1;

    if (dx != 0 || dy != 0) {
        TryMove(dx, dy);
        moveCooldown_ = 1.0f / GetSpeed();
    }
}

void Player::OnKeyPress(const std::string& key) {
    if (key == "space") MeleeAttack();
}

void Player::MeleeAttack() {
    // Hit the nearest living unit within attack range.
    Unit* target = GetWorld()->FindNearestUnitInRange(this, GetAttackRange());
    if (!target) {
        GetWorld()->AddMessage("You swing at nothing.");
        return;
    }
    DamageInfo dmg(GetAttackPower(), DamageSchool::Physical);
    DealDamage(target, dmg);
}

void Player::OnDamageTaken(Unit* attacker, DamageInfo* damage) {
    (void)attacker;
    GetWorld()->AddMessage("You take " + std::to_string(damage->GetAmount()) + " damage!");
}

void Player::Kill(Unit* killer) {
    // Don't Destroy() the player — leave a corpse so the HUD/world stay valid.
    health_ = 0;
    OnDeath(killer);
}

void Player::OnDeath(Unit* killer) {
    (void)killer;
    GetWorld()->AddMessage("You died. (Esc to quit)");
    sprite_ = Sprite(SpriteId::PlayerDead);   // corpse sprite
}

void Player::OnKill(Unit* victim) {
    GetWorld()->AddMessage("You slay " + victim->GetName() + "!");
}
