#include "Engine/SpriteManager.h"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdio>

namespace {
// SpriteId -> filename (relative to assets/sprites). Keep in sync with SpriteId.
const char* fileFor(SpriteId id) {
    switch (id) {
        case SpriteId::Floor:      return "floor.png";
        case SpriteId::Wall:       return "wall.png";
        case SpriteId::Player:     return "player.png";
        case SpriteId::PlayerDead: return "player_dead.png";
        case SpriteId::Grunt:      return "grunt.png";
        case SpriteId::Chest:      return "chest.png";
        case SpriteId::FireRune0:  return "firerune_0.png";
        case SpriteId::FireRune1:  return "firerune_1.png";
        case SpriteId::FireRune2:  return "firerune_2.png";
        default:                   return nullptr;
    }
}
} // namespace

bool SpriteManager::loadAll(const std::string& assetsDir) {
    stbi_set_flip_vertically_on_load(false);
    bool ok = true;

    for (int i = 1; i < (int)SpriteId::Count; ++i) {
        SpriteId id = (SpriteId)i;
        const char* file = fileFor(id);
        if (!file) continue;

        std::string path = assetsDir + "/sprites/" + file;
        int w = 0, h = 0, ch = 0;
        unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &ch, 4);
        if (!pixels) {
            std::fprintf(stderr, "[sprite] failed to load %s\n", path.c_str());
            ok = false;
            continue;
        }

        unsigned int tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        stbi_image_free(pixels);
        textures_[i] = Tex{ tex, w, h };
    }
    return ok;
}

void SpriteManager::shutdown() {
    for (auto& t : textures_) {
        if (t.id) glDeleteTextures(1, &t.id);
        t = Tex{};
    }
}

unsigned int SpriteManager::texture(SpriteId id) const {
    int i = (int)id;
    return (i > 0 && i < (int)SpriteId::Count) ? textures_[i].id : 0;
}
int SpriteManager::width(SpriteId id) const {
    int i = (int)id;
    return (i > 0 && i < (int)SpriteId::Count) ? textures_[i].w : 0;
}
int SpriteManager::height(SpriteId id) const {
    int i = (int)id;
    return (i > 0 && i < (int)SpriteId::Count) ? textures_[i].h : 0;
}
