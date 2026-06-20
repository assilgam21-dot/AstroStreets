#include "Engine/SpriteManager.h"
#include "Engine/D3DUtil.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdio>

namespace {
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
}

bool SpriteManager::loadAll(ID3D12Device* device, ID3D12GraphicsCommandList* cmd,
                            ID3D12DescriptorHeap* srvHeap, UINT srvDescSize, UINT& nextSlot,
                            const std::string& assetsDir,
                            std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>& uploadsKeepAlive) {
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

        Tex t;
        t.res = d3dutil::UploadTexture(device, cmd, (UINT)w, (UINT)h,
                                       DXGI_FORMAT_R8G8B8A8_UNORM, 4, pixels, uploadsKeepAlive);
        t.gpu = d3dutil::CreateSRVAt(device, srvHeap, srvDescSize, nextSlot, t.res.Get(),
                                     DXGI_FORMAT_R8G8B8A8_UNORM);
        t.w = w; t.h = h; t.valid = true;
        ++nextSlot;

        stbi_image_free(pixels);
        textures_[i] = std::move(t);
    }
    return ok;
}

bool SpriteManager::has(SpriteId id) const {
    int i = (int)id;
    return i > 0 && i < (int)SpriteId::Count && textures_[i].valid;
}
D3D12_GPU_DESCRIPTOR_HANDLE SpriteManager::handle(SpriteId id) const {
    return textures_[(int)id].gpu;
}
int SpriteManager::width(SpriteId id) const {
    int i = (int)id;
    return (i > 0 && i < (int)SpriteId::Count) ? textures_[i].w : 0;
}
int SpriteManager::height(SpriteId id) const {
    int i = (int)id;
    return (i > 0 && i < (int)SpriteId::Count) ? textures_[i].h : 0;
}
