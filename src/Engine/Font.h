#pragma once
#include <vector>
#include <d3d12.h>
#include <wrl/client.h>

Microsoft::WRL::ComPtr<ID3D12Resource> CreateFontAtlas(
    ID3D12Device* dev, ID3D12GraphicsCommandList* cmd,
    ID3D12DescriptorHeap* srvHeap, UINT srvDescSize, UINT slot,
    D3D12_GPU_DESCRIPTOR_HANDLE& outGpu,
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>& uploadsKeepAlive);
