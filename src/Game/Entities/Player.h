#pragma once
#include <string>
#include "Game/Entities/Unit.h"

// The player avatar. Reads held keys for movement and discrete presses for
// actions (a melee attack for now; spells will come from Lua later).
class Player : public Unit {
public:
    explicit Player(World* world);

    EntityType GetType() const override { return EntityType::Player; }

    void OnSpawn() override;
    void OnUpdate(float dt) override;            // movement from held keys
    void OnKeyPress(const std::string& key);     // discrete actions

    void OnDamageTaken(Unit* attacker, DamageInfo* damage) override;
    void OnDeath(Unit* killer) override;
    void OnKill(Unit* victim) override;

    // The player's corpse stays on the field instead of being culled.
    void Kill(Unit* killer) override;

private:
    void TryMove(int dx, int dy);
    void MeleeAttack();

    float moveCooldown_ = 0.0f;
};
