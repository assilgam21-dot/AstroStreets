#pragma once
#include <array>
#include <string>
#include "Core/SpriteId.h"

// Loads sprite PNGs into GL textures and resolves SpriteId -> texture id.
// Render thread only (it touches GL). The id->filename table lives in the .cpp.
class SpriteManager {
public:
    bool loadAll(const std::string& assetsDir);   // assumes a current GL context
    void shutdown();

    unsigned int texture(SpriteId id) const;       // 0 if unloaded/None
    int width(SpriteId id) const;
    int height(SpriteId id) const;

private:
    struct Tex { unsigned int id = 0; int w = 0, h = 0; };
    std::array<Tex, (size_t)SpriteId::Count> textures_{};
};
