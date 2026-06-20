#pragma once
#include <array>
#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl/client.h>
#include "Core/SpriteId.h"

class SpriteManager {
public:
    bool loadAll(ID3D12Device* device, ID3D12GraphicsCommandList* cmd,
                 ID3D12DescriptorHeap* srvHeap, UINT srvDescSize, UINT& nextSlot,
                 const std::string& assetsDir,
                 std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>& uploadsKeepAlive);

    bool has(SpriteId id) const;
    D3D12_GPU_DESCRIPTOR_HANDLE handle(SpriteId id) const;
    int width(SpriteId id) const;
    int height(SpriteId id) const;

private:
    struct Tex {
        Microsoft::WRL::ComPtr<ID3D12Resource> res;
        D3D12_GPU_DESCRIPTOR_HANDLE gpu{ 0 };
        int w = 0, h = 0;
        bool valid = false;
    };
    std::array<Tex, (size_t)SpriteId::Count> textures_{};
};
