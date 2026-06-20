#include "Game/Entities/ScriptedAreaTrigger.h"

#include "Game/Scripting/ScriptEngine.h"
#include "Game/Entities/Unit.h"
#include "Game/World.h"
#include "Core/SpriteId.h"
#include "Core/RenderFrame.h"
#include "Core/Constants.h"

#include <cmath>

int ScriptedAreaTrigger::DimW(sol::table& def) { return def.get_or("width", 1); }
int ScriptedAreaTrigger::DimH(sol::table& def) { return def.get_or("height", 1); }

ScriptedAreaTrigger::ScriptedAreaTrigger(World* world, ScriptEngine* engine, sol::table def)
    : AreaTrigger(world, DimW(def), DimH(def)), engine_(engine), def_(def) {
    SetName(def_.get_or("name", std::string("ScriptedAreaTrigger")));

    sol::optional<sol::table> frames = def_["frames"];
    if (frames) {
        std::size_t n = frames->size();
        for (std::size_t i = 1; i <= n; ++i) {
            sol::table f = (*frames)[i];
            int id = f.get_or("sprite", 0);
            float dur = f.get_or("dur", 0.1f);
            animation_.addFrame(Sprite(static_cast<SpriteId>(id)), dur);
        }
    }

    BuildInstance();
}

void ScriptedAreaTrigger::BuildInstance() {
    sol::state_view lua = engine_->Lua();
    inst_ = lua.create_table();

    sol::table mt = lua.create_table();
    mt["__index"] = def_;
    inst_[sol::metatable_key] = mt;

    inst_["unitsInside"] = [this]() {
        sol::state_view lua = engine_->Lua();
        sol::table out = lua.create_table();
        int i = 1;
        for (Unit* u : GetWorld()->GetUnits())
            if (u->IsAlive() && Contains(u->GetX(), u->GetY()))
                out[i++] = u;
        return out;
    };
    inst_["log"] = [this](const std::string& msg) {
        GetWorld()->AddMessage(msg);
    };
}

void ScriptedAreaTrigger::OnSpawn() {
    Call("onSpawn");
}

void ScriptedAreaTrigger::OnUpdate(float dt) {
    phase_ += dt;
    AreaTrigger::OnUpdate(dt);
    Call("onUpdate", dt);
}

void ScriptedAreaTrigger::EmitLights(RenderFrame& frame) const {
    sol::optional<sol::table> light = def_["light"];
    if (!light) return;

    float radiusTiles = light->get_or("radius", 0.0f);
    if (radiusTiles <= 0.0f) return;

    float intensity = light->get_or("intensity", 1.0f);
    float flicker = 0.86f + 0.14f * std::sin(phase_ * 11.0f);

    LightDraw l;
    l.x = (GetX() + GetWidth() * 0.5f) * TILE_SIZE;
    l.y = (GetY() + GetHeight() * 0.5f) * TILE_SIZE;
    l.r = light->get_or("r", 1.0f);
    l.g = light->get_or("g", 1.0f);
    l.b = light->get_or("b", 1.0f);
    l.radius = radiusTiles * TILE_SIZE;
    l.intensity = intensity * flicker;
    frame.lights.push_back(l);
}

void ScriptedAreaTrigger::OnUnitEnter(Unit* unit) {
    Call("onEnter", unit);
}

void ScriptedAreaTrigger::OnUnitExit(Unit* unit) {
    Call("onExit", unit);
}

void ScriptedAreaTrigger::OnUnitStay(Unit* unit, float dt) {
    Call("onStay", unit, dt);
}
