#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include "Engine/SpriteManager.h"

struct RenderFrame;

class Renderer {
public:
    bool init(void* hwnd, int width, int height, const std::string& assetsDir);
    void shutdown();
    void draw(const RenderFrame& frame, int viewportW, int viewportH);

private:
    static const UINT kFrameCount = 2;

    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    bool createDevice();
    bool createSwapChain(void* hwnd);
    bool createTargets();
    bool createSync();
    bool createRootSignature();
    bool createPipelines();
    bool createBuffers();
    bool loadResources(const std::string& assetsDir);

    void flushGpu();
    void writeScene(const RenderFrame& frame);

    ComPtr<IDXGIFactory4>             factory_;
    ComPtr<ID3D12Device>              device_;
    ComPtr<ID3D12CommandQueue>        queue_;
    ComPtr<IDXGISwapChain3>           swapChain_;
    ComPtr<ID3D12DescriptorHeap>      rtvHeap_;
    ComPtr<ID3D12Resource>            renderTargets_[kFrameCount];
    ComPtr<ID3D12CommandAllocator>    allocator_;
    ComPtr<ID3D12GraphicsCommandList> cmd_;
    ComPtr<ID3D12RootSignature>       rootSig_;
    ComPtr<ID3D12PipelineState>       spritePso_;
    ComPtr<ID3D12PipelineState>       textPso_;
    ComPtr<ID3D12DescriptorHeap>      srvHeap_;
    ComPtr<ID3D12Resource>            cbuffer_;
    ComPtr<ID3D12Resource>            vbuffer_;
    ComPtr<ID3D12Resource>            fontTex_;
    ComPtr<ID3D12Fence>              fence_;

    UINT64 fenceValue_ = 0;
    void*  fenceEvent_ = nullptr;

    UINT rtvDescSize_ = 0;
    UINT srvDescSize_ = 0;
    UINT frameIndex_ = 0;

    int width_ = 0, height_ = 0;

    SpriteManager sprites_;
    D3D12_GPU_DESCRIPTOR_HANDLE fontGpu_{ 0 };

    uint8_t* cbMapped_ = nullptr;
    uint8_t* vbMapped_ = nullptr;
    size_t   vbCapacityBytes_ = 0;
    D3D12_GPU_VIRTUAL_ADDRESS vbGpuBase_ = 0;
    D3D12_GPU_VIRTUAL_ADDRESS cbGpuBase_ = 0;

    std::vector<float> scratch_;
};
