#pragma once
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Game/Entities/Entity.h"
#include "Game/Camera.h"

struct RenderFrame;
class Input;
class Unit;
class Player;

// The authoritative C++ game state, owned and driven by the simulation thread.
//
// It owns every Entity, a wall map, the Camera, and a message log, and provides
// the spatial queries entities use. Each tick it routes input, updates entities,
// culls the dead and steps the camera; Render() emits a RenderFrame (sprite draw
// list + HUD text + camera state) for the render thread.
class World {
public:
    World(Input& input, int cols, int rows, int viewportW, int viewportH, std::string scriptsDir);
    ~World();

    void Setup();
    void Update(float dt);
    void Render(RenderFrame& frame);

    // ---- spawning ----
    template <class T, class... Args>
    T* Spawn(Args&&... args) {
        auto owned = std::make_unique<T>(this, std::forward<Args>(args)...);
        T* ptr = owned.get();
        entities_.push_back(std::move(owned));
        ptr->OnSpawn();
        return ptr;
    }

    // ---- accessors ----
    Input&  GetInput()  { return input_; }
    Player* GetPlayer() { return player_; }
    Camera& GetCamera() { return camera_; }
    int     GetCols() const { return cols_; }
    int     GetRows() const { return rows_; }

    // ---- map ----
    bool IsWall(int x, int y) const;
    void SetWall(int x, int y, bool wall);

    // ---- queries ----
    std::vector<Unit*> GetUnits() const;
    Unit* FindNearestUnitInRange(const Unit* from, int range) const;

    // ---- ui ----
    void AddMessage(const std::string& text);

private:
    void HandleInput();        // camera hotkeys + route the rest to the player

    Input& input_;
    int    cols_, rows_;
    int    viewW_, viewH_;
    std::string scriptsDir_;   // reserved for the Lua spell layer (later)

    std::vector<std::unique_ptr<Entity>> entities_;
    Player* player_ = nullptr;

    Camera camera_;
    float  focusX_ = 0, focusY_ = 0;   // a point of interest (the fire rune)

    std::vector<bool>       walls_;
    std::deque<std::string> log_;
};
