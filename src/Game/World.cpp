#include "Game/World.h"

#include "Game/Entities/Unit.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/Npc.h"
#include "Game/Entities/GameObject.h"
#include "Game/Entities/FireRune.h"

#include "Core/RenderFrame.h"
#include "Core/Constants.h"
#include "Core/Input.h"

#include <climits>

namespace {
const size_t kMaxLog = 5;
float TileCenterPx(int tile) { return (tile + 0.5f) * TILE_SIZE; }
}

World::World(Input& input, int cols, int rows, int viewportW, int viewportH, std::string scriptsDir)
    : input_(input), cols_(cols), rows_(rows), viewW_(viewportW), viewH_(viewportH),
      scriptsDir_(std::move(scriptsDir)) {
    walls_.assign(size_t(cols_) * size_t(rows_), false);
}

World::~World() = default;

void World::SetWall(int x, int y, bool wall) {
    if (x < 0 || y < 0 || x >= cols_ || y >= rows_) return;
    walls_[size_t(y) * size_t(cols_) + size_t(x)] = wall;
}

bool World::IsWall(int x, int y) const {
    if (x < 1 || y < 1 || x >= cols_ - 1 || y >= rows_ - 1) return true;  // border
    return walls_[size_t(y) * size_t(cols_) + size_t(x)];
}

void World::AddMessage(const std::string& text) {
    log_.push_front(text);
    while (log_.size() > kMaxLog) log_.pop_back();
}

std::vector<Unit*> World::GetUnits() const {
    std::vector<Unit*> units;
    for (const auto& e : entities_) {
        EntityType t = e->GetType();
        if (t == EntityType::Unit || t == EntityType::Player || t == EntityType::Npc)
            units.push_back(static_cast<Unit*>(e.get()));
    }
    return units;
}

Unit* World::FindNearestUnitInRange(const Unit* from, int range) const {
    Unit* best = nullptr;
    int bestDist = INT_MAX;
    for (Unit* u : GetUnits()) {
        if (u == from || !u->IsAlive()) continue;
        int d = from->DistanceTo(u);
        if (d <= range && d < bestDist) { best = u; bestDist = d; }
    }
    return best;
}

void World::Setup() {
    for (int x = 10; x <= 22; ++x) SetWall(x, 10, true);
    for (int y = 10; y <= 18; ++y) SetWall(34, y, true);

    player_ = Spawn<Player>();
    player_->SetPosition(8, 8);

    Npc* a = Spawn<Npc>(); a->SetName("Red Grunt");    a->SetPosition(44, 22);
    Npc* b = Spawn<Npc>(); b->SetName("Orange Grunt"); b->SetPosition(52, 12);
    b->GetAnimation() = Animation{};   // recolor the second grunt to orange
    b->GetAnimation()
        .addFrame(Sprite(SpriteId::Grunt, 1.0f, 0.70f, 0.25f), 0.5f)
        .addFrame(Sprite(SpriteId::Grunt, 0.85f, 0.55f, 0.20f), 0.5f);

    GameObject* chest = Spawn<GameObject>();
    chest->SetName("Chest");
    chest->SetPosition(15, 25);

    FireRune* rune = Spawn<FireRune>(5, 3);
    rune->SetPosition(26, 26);
    focusX_ = (26 + 5 * 0.5f) * TILE_SIZE;   // rune center, for the focus camera
    focusY_ = (26 + 3 * 0.5f) * TILE_SIZE;

    camera_.SetFollow();
    AddMessage("Cam: 1=follow 2=lock 3/f=focus  z/x=zoom");
}

void World::HandleInput() {
    for (auto& key : input_.drainPresses()) {
        if      (key == "1")               camera_.SetFollow();
        else if (key == "2")               camera_.SetLocked();
        else if (key == "3" || key == "f") camera_.FocusOn(focusX_, focusY_, 1.8f, 0.6f, 1.5f, true);
        else if (key == "z")               camera_.AddZoom(+0.2f);
        else if (key == "x")               camera_.AddZoom(-0.2f);
        else if (player_)                  player_->OnKeyPress(key);
    }
}

void World::Update(float dt) {
    HandleInput();

    for (auto& e : entities_)
        e->Update(dt);

    for (size_t i = entities_.size(); i-- > 0; ) {
        if (!entities_[i]->IsAlive()) {
            entities_[i]->OnDestroy();
            entities_.erase(entities_.begin() + i);
        }
    }

    float tx = focusX_, ty = focusY_;   // fallback target if no player
    if (player_) { tx = TileCenterPx(player_->GetX()); ty = TileCenterPx(player_->GetY()); }
    camera_.Update(dt, tx, ty);
}

void World::Render(RenderFrame& frame) {
    frame.clear();

    // ground / walls
    for (int y = 0; y < rows_; ++y) {
        for (int x = 0; x < cols_; ++x) {
            SpriteId tile = IsWall(x, y) ? SpriteId::Wall : SpriteId::Floor;
            frame.sprites.push_back(SpriteDraw{
                tile, float(x * TILE_SIZE), float(y * TILE_SIZE),
                float(TILE_SIZE), float(TILE_SIZE), 1, 1, 1, 1 });
        }
    }

    // entities — area triggers under units
    for (const auto& e : entities_)
        if (e->GetType() == EntityType::AreaTrigger) e->Render(frame);
    for (const auto& e : entities_)
        if (e->GetType() != EntityType::AreaTrigger) e->Render(frame);

    // camera
    frame.camera = camera_.State();

    // HUD (screen space)
    if (player_) {
        std::string hud = "HP " + std::to_string(player_->GetHealth()) + "/" +
                          std::to_string(player_->GetMaxHealth()) +
                          "   pos " + std::to_string(player_->GetX()) + "," +
                          std::to_string(player_->GetY());
        frame.texts.push_back(TextDraw{ 8, 8, hud, 0.6f, 1.0f, 0.6f });
    }
    const char* modeName =
        camera_.Mode() == CameraMode::Follow ? "follow" :
        camera_.Mode() == CameraMode::Locked ? "locked" : "focus";
    frame.texts.push_back(TextDraw{ 8, 30,
        std::string("cam ") + modeName, 0.6f, 0.8f, 1.0f });

    int y = viewH_ - 24;
    for (const auto& line : log_) {
        frame.texts.push_back(TextDraw{ 8, y, line, 0.85f, 0.85f, 0.6f });
        y -= 18;
    }
}
