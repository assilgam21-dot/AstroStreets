#pragma once
#include <cstdint>
#include <string>
#include "Core/Sprite.h"
#include "Core/Animation.h"

class World;
struct RenderFrame;

// What kind of entity this is — cheap RTTI for gameplay queries.
enum class EntityType {
    Entity,
    Unit,
    Player,
    Npc,
    GameObject,
    AreaTrigger,
};

// Base class for everything that lives in the World and has a position and a
// visual. It owns a Sprite and an Animation (the "visual attributes"), and
// exposes lifecycle hooks subclasses override. The engine drives it through
// Update() (advances animation + calls OnUpdate) and Render().
class Entity {
public:
    explicit Entity(World* world);
    virtual ~Entity();

    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;

    // ---- identity ----
    uint32_t GetId() const { return id_; }
    virtual EntityType GetType() const { return EntityType::Entity; }
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }

    // ---- position (grid coordinates) ----
    int GetX() const { return x_; }
    int GetY() const { return y_; }
    void SetPosition(int x, int y) { x_ = x; y_ = y; }

    // ---- visual ----
    Sprite&         GetSprite()        { return sprite_; }
    const Sprite&   GetSprite() const  { return sprite_; }
    Animation&      GetAnimation()     { return animation_; }

    // ---- lifecycle ----
    bool IsAlive() const { return alive_; }
    void Destroy() { alive_ = false; }   // marked here, removed by the World
    World* GetWorld() const { return world_; }

    // ---- hooks (override these) ----
    virtual void OnSpawn() {}
    virtual void OnUpdate(float /*dt*/) {}
    virtual void OnDestroy() {}

    // ---- engine-driven (usually not overridden) ----
    void Update(float dt);                        // advance animation, call OnUpdate
    virtual void Render(RenderFrame& frame) const; // emit sprite draw(s)

protected:
    World*      world_ = nullptr;
    uint32_t    id_ = 0;
    std::string name_;
    int         x_ = 0, y_ = 0;
    Sprite      sprite_;
    Animation   animation_;
    bool        alive_ = true;
};
