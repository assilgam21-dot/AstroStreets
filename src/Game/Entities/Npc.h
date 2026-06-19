#pragma once
#include "Game/Entities/Unit.h"

// A hostile NPC. Wanders until it sees the player, then chases and bites.
// Demonstrates an AI handler (OnUpdate) plus combat reactions (OnDamageTaken /
// OnDeath) and a per-unit idle animation.
class Npc : public Unit {
public:
    explicit Npc(World* world);

    EntityType GetType() const override { return EntityType::Npc; }

    void OnUpdate(float dt) override;
    void OnDamageTaken(Unit* attacker, DamageInfo* damage) override;
    void OnDeath(Unit* killer) override;

private:
    void TryMove(int dx, int dy);

    int   sightRange_ = 8;
    float stepCooldown_ = 0.0f;
    float biteCooldown_ = 0.0f;
};
