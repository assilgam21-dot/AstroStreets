#include "Game/Entities/Entity.h"
#include "Core/RenderFrame.h"
#include "Core/Constants.h"

namespace {
uint32_t g_nextEntityId = 0;   // sim thread only — no synchronization needed
}

Entity::Entity(World* world) : world_(world) {
    id_ = ++g_nextEntityId;
}

Entity::~Entity() = default;

void Entity::Update(float dt) {
    if (!animation_.empty()) animation_.update(dt);
    OnUpdate(dt);
}

void Entity::Render(RenderFrame& frame) const {
    const Sprite& s = animation_.empty() ? sprite_ : animation_.current();
    if (s.id == SpriteId::None) return;
    frame.sprites.push_back(SpriteDraw{
        s.id,
        float(x_ * TILE_SIZE), float(y_ * TILE_SIZE),
        float(TILE_SIZE), float(TILE_SIZE),
        s.r, s.g, s.b, s.a });
}
