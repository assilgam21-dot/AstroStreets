#include "Game/Scripting/ScriptEngine.h"

#include "Game/World.h"
#include "Game/Entities/Unit.h"
#include "Game/Entities/DamageInfo.h"
#include "Core/SpriteId.h"

#include <cstdio>

ScriptEngine::ScriptEngine(World* world) : world_(world) {}

bool ScriptEngine::Init(const std::string& scriptsDir) {
    scriptsDir_ = scriptsDir;
    lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string);
    Bind();
    return true;
}

void ScriptEngine::Bind() {
    lua_.new_usertype<Unit>("Unit",
        sol::no_constructor,
        "name",       [](Unit& u) { return u.GetName(); },
        "x",          [](Unit& u) { return u.GetX(); },
        "y",          [](Unit& u) { return u.GetY(); },
        "isAlive",    [](Unit& u) { return u.IsAlive(); },
        "health",     [](Unit& u) { return u.GetHealth(); },
        "maxHealth",  [](Unit& u) { return u.GetMaxHealth(); },
        "takeDamage", [](Unit& u, int amount, int school) {
            DamageInfo dmg(amount, static_cast<DamageSchool>(school));
            u.TakeDamage(dmg);
        });

    lua_["DamageSchool"] = lua_.create_table_with(
        "Physical", static_cast<int>(DamageSchool::Physical),
        "Fire",     static_cast<int>(DamageSchool::Fire),
        "Frost",    static_cast<int>(DamageSchool::Frost),
        "Arcane",   static_cast<int>(DamageSchool::Arcane),
        "Nature",   static_cast<int>(DamageSchool::Nature),
        "Shadow",   static_cast<int>(DamageSchool::Shadow),
        "Holy",     static_cast<int>(DamageSchool::Holy));

    lua_["Sprite"] = lua_.create_table_with(
        "None",       static_cast<int>(SpriteId::None),
        "Floor",      static_cast<int>(SpriteId::Floor),
        "Wall",       static_cast<int>(SpriteId::Wall),
        "Player",     static_cast<int>(SpriteId::Player),
        "PlayerDead", static_cast<int>(SpriteId::PlayerDead),
        "Grunt",      static_cast<int>(SpriteId::Grunt),
        "Chest",      static_cast<int>(SpriteId::Chest),
        "FireRune0",  static_cast<int>(SpriteId::FireRune0),
        "FireRune1",  static_cast<int>(SpriteId::FireRune1),
        "FireRune2",  static_cast<int>(SpriteId::FireRune2));
}

sol::table ScriptEngine::LoadDefinition(const std::string& relPath) {
    std::string path = scriptsDir_ + "/" + relPath;
    sol::protected_function_result result = lua_.safe_script_file(path, &sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        std::fprintf(stderr, "Lua error loading %s: %s\n", path.c_str(), err.what());
        return lua_.create_table();
    }
    sol::object obj = result;
    if (obj.is<sol::table>()) return obj.as<sol::table>();
    std::fprintf(stderr, "Script %s did not return a table\n", path.c_str());
    return lua_.create_table();
}
