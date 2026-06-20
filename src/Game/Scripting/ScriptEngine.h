#pragma once
#include <string>
#include <sol/sol.hpp>

class World;

class ScriptEngine {
public:
    explicit ScriptEngine(World* world);

    bool Init(const std::string& scriptsDir);
    sol::table LoadDefinition(const std::string& relPath);

    sol::state& Lua() { return lua_; }
    World* GetWorld() const { return world_; }
    const std::string& Dir() const { return scriptsDir_; }

private:
    void Bind();

    sol::state  lua_;
    World*      world_ = nullptr;
    std::string scriptsDir_;
};
