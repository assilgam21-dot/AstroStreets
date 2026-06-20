#pragma once
#include <cstdio>
#include <utility>
#include <sol/sol.hpp>
#include "Game/Entities/AreaTrigger.h"

class ScriptEngine;

class ScriptedAreaTrigger : public AreaTrigger {
public:
    ScriptedAreaTrigger(World* world, ScriptEngine* engine, sol::table def);

    void OnSpawn() override;
    void OnUpdate(float dt) override;
    void OnUnitEnter(Unit* unit) override;
    void OnUnitExit(Unit* unit) override;
    void OnUnitStay(Unit* unit, float dt) override;
    void EmitLights(RenderFrame& frame) const override;

private:
    static int DimW(sol::table& def);
    static int DimH(sol::table& def);
    void BuildInstance();

    template <class... A>
    void Call(const char* name, A&&... args) {
        sol::protected_function fn = def_[name];
        if (!fn.valid()) return;
        sol::protected_function_result r = fn(inst_, std::forward<A>(args)...);
        if (!r.valid()) {
            sol::error e = r;
            std::fprintf(stderr, "Lua hook %s error: %s\n", name, e.what());
        }
    }

    ScriptEngine* engine_ = nullptr;
    sol::table    def_;
    sol::table    inst_;
    float         phase_ = 0.0f;
};
