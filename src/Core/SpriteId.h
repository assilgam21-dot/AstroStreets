#pragma once

// A stable handle for a sprite asset, shared between the simulation thread
// (which references sprites by id) and the render thread (which maps each id to
// a loaded GL texture in SpriteManager). Keep this in sync with the file table
// in SpriteManager.cpp.
enum class SpriteId : int {
    None = 0,
    Floor,
    Wall,
    Player,
    PlayerDead,
    Grunt,
    Chest,
    FireRune0,
    FireRune1,
    FireRune2,
    Count
};
