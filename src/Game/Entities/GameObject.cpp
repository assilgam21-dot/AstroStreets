#include "Game/Entities/GameObject.h"

GameObject::GameObject(World* world) : Entity(world) {
    SetName("Object");
    // a gentle shimmer so static objects feel alive (animation attribute)
    GetAnimation()
        .addFrame(Sprite(SpriteId::Chest, 1.0f, 1.0f, 1.0f), 0.6f)
        .addFrame(Sprite(SpriteId::Chest, 1.0f, 0.9f, 0.7f), 0.4f);
}
