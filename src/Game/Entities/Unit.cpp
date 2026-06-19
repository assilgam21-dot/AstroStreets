#include "Game/Entities/Unit.h"
#include "Game/Entities/DamageInfo.h"

#include <algorithm>
#include <climits>
#include <cstdlib>

Unit::Unit(World* world) : Entity(world) {}

void Unit::SetHealth(int health) {
    health_ = std::clamp(health, 0, maxHealth_);
}

void Unit::SetMaxHealth(int maxHealth) {
    maxHealth_ = std::max(1, maxHealth);
    if (health_ > maxHealth_) health_ = maxHealth_;
}

int Unit::DistanceTo(const Entity* other) const {
    if (!other) return INT_MAX;
    return std::max(std::abs(other->GetX() - x_), std::abs(other->GetY() - y_));
}

bool Unit::IsInRange(const Entity* other, int range) const {
    return other && DistanceTo(other) <= range;
}

void Unit::DealDamage(Unit* target, DamageInfo& damage) {
    if (!target || !target->IsAlive()) return;
    damage.SetAttacker(this);
    target->TakeDamage(damage);
}

void Unit::TakeDamage(DamageInfo& damage) {
    if (!IsAlive()) return;

    // Let the target react/modify before the hit lands.
    OnDamageTaken(damage.GetAttacker(), &damage);

    int amount = damage.GetAmount();
    if (amount <= 0) return;

    health_ = std::max(0, health_ - amount);

    if (health_ == 0) {
        Unit* killer = damage.GetAttacker();
        Kill(killer);
        if (killer) killer->OnKill(this);
    }
}

void Unit::Heal(int amount, Unit* healer) {
    if (!IsAlive() || amount <= 0) return;
    health_ = std::min(maxHealth_, health_ + amount);
    OnHeal(healer, amount);
}

void Unit::Kill(Unit* killer) {
    health_ = 0;
    OnDeath(killer);
    Destroy();
}
