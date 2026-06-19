#pragma once

class Unit;

// How damage is typed. Extend freely as the combat model grows.
enum class DamageSchool {
    Physical,
    Fire,
    Frost,
    Arcane,
    Nature,
    Shadow,
    Holy,
};

// A mutable packet describing one instance of damage as it flows from attacker
// to target. It is passed by pointer into OnDamageTaken so handlers can inspect
// and modify it (absorb, reduce, change school) before it is applied.
class DamageInfo {
public:
    DamageInfo() = default;
    DamageInfo(int amount, DamageSchool school = DamageSchool::Physical)
        : amount_(amount), school_(school) {}

    int  GetAmount() const { return amount_; }
    void SetAmount(int amount) { amount_ = amount < 0 ? 0 : amount; }
    void Absorb(int amount) { amount_ = (amount >= amount_) ? 0 : amount_ - amount; }

    DamageSchool GetSchool() const { return school_; }
    void         SetSchool(DamageSchool school) { school_ = school; }

    Unit* GetAttacker() const { return attacker_; }
    void  SetAttacker(Unit* attacker) { attacker_ = attacker; }

private:
    int          amount_ = 0;
    DamageSchool school_ = DamageSchool::Physical;
    Unit*        attacker_ = nullptr;
};
