#include "Engine/Font.h"
#include "Engine/font8x8_basic.h"
#include "Engine/D3DUtil.h"

Microsoft::WRL::ComPtr<ID3D12Resource> CreateFontAtlas(
    ID3D12Device* dev, ID3D12GraphicsCommandList* cmd,
    ID3D12DescriptorHeap* srvHeap, UINT srvDescSize, UINT slot,
    D3D12_GPU_DESCRIPTOR_HANDLE& outGpu,
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>& uploadsKeepAlive) {

    const int AW = 16 * 8, AH = 8 * 8;
    std::vector<unsigned char> pixels(size_t(AW) * size_t(AH), 0);

    for (int c = 0; c < 128; ++c) {
        int gx0 = (c % 16) * 8, gy0 = (c / 16) * 8;
        for (int row = 0; row < 8; ++row) {
            unsigned char bits = (unsigned char)font8x8_basic[c][row];
            for (int col = 0; col < 8; ++col)
                if (bits & (1 << col))
                    pixels[size_t(gy0 + row) * AW + (gx0 + col)] = 255;
        }
    }

    auto tex = d3dutil::UploadTexture(dev, cmd, AW, AH, DXGI_FORMAT_R8_UNORM, 1,
                                      pixels.data(), uploadsKeepAlive);
    outGpu = d3dutil::CreateSRVAt(dev, srvHeap, srvDescSize, slot, tex.Get(), DXGI_FORMAT_R8_UNORM);
    return tex;
}
