#pragma once
#include "Game/Entities/Entity.h"

class DamageInfo;

// Base class for any entity that lives, moves and fights: players and NPCs.
// Holds the common combat/movement stats and the combat pipeline. Subclasses
// override the On* hooks to react. (Later these hooks can also forward to Lua.)
class Unit : public Entity {
public:
    explicit Unit(World* world);

    EntityType GetType() const override { return EntityType::Unit; }

    // ---- stats ----
    int  GetHealth() const { return health_; }
    void SetHealth(int health);
    int  GetMaxHealth() const { return maxHealth_; }
    void SetMaxHealth(int maxHealth);

    float GetSpeed() const { return speed_; }            // tiles per second
    void  SetSpeed(float speed) { speed_ = speed; }

    int  GetAttackRange() const { return attackRange_; } // in tiles
    void SetAttackRange(int range) { attackRange_ = range; }

    int  GetAttackPower() const { return attackPower_; }
    void SetAttackPower(int power) { attackPower_ = power; }

    bool IsAlive() const { return alive_ && health_ > 0; }

    // ---- spatial helpers ----
    int  DistanceTo(const Entity* other) const;          // Chebyshev (grid) distance
    bool IsInRange(const Entity* other, int range) const;

    // ---- combat pipeline ----
    void DealDamage(Unit* target, DamageInfo& damage);   // attacker side
    void TakeDamage(DamageInfo& damage);                 // target side (applies + hooks)
    void Heal(int amount, Unit* healer = nullptr);
    virtual void Kill(Unit* killer);

    // ---- hooks (override in subclasses) ----
    virtual void OnDamageTaken(Unit* attacker, DamageInfo* damage) { (void)attacker; (void)damage; }
    virtual void OnDeath(Unit* killer) { (void)killer; }
    virtual void OnHeal(Unit* healer, int amount) { (void)healer; (void)amount; }
    virtual void OnKill(Unit* victim) { (void)victim; }   // we killed something

protected:
    int   health_ = 1;
    int   maxHealth_ = 1;
    float speed_ = 1.0f;
    int   attackRange_ = 1;
    int   attackPower_ = 1;
};
