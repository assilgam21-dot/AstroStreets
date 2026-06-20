#include "Engine/Renderer.h"
#include "Engine/Font.h"
#include "Engine/D3DUtil.h"
#include "Core/RenderFrame.h"

#include <d3dcompiler.h>
#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

using Microsoft::WRL::ComPtr;

namespace {

constexpr int kGlyph = 8;
constexpr int kTextScale = 2;
constexpr int kMaxLights = 16;
constexpr UINT kVertexStride = 8 * sizeof(float);

struct SceneCB {
    float camPos[2]; float zoom; float pad0;
    float viewport[2]; float pad1[2];
    float ambient[4];
    int lightCount; int pad2[3];
    float lights[kMaxLights * 2][4];
};

UINT64 Align(UINT64 v, UINT64 a) { return (v + a - 1) & ~(a - 1); }

bool CK(HRESULT hr, const char* what) {
    if (FAILED(hr)) { std::fprintf(stderr, "[d3d12] %s failed (0x%08lx)\n", what, (unsigned long)hr); return false; }
    return true;
}

const char* kHlsl = R"(
cbuffer Scene : register(b0) {
    float2 camPos; float zoom; float _p0;
    float2 viewport; float2 _p1;
    float4 ambient;
    int lightCount; int3 _p2;
    float4 lights[32];
};
Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

struct VSIn { float2 pos:POSITION; float2 uv:TEXCOORD; float4 col:COLOR; };
struct PSIn { float4 pos:SV_POSITION; float2 uv:TEXCOORD0; float4 col:COLOR0; float2 world:TEXCOORD1; };

PSIn VSSprite(VSIn i) {
    PSIn o;
    float sx = (i.pos.x - camPos.x) * zoom + viewport.x * 0.5;
    float sy = (i.pos.y - camPos.y) * zoom + viewport.y * 0.5;
    o.pos = float4(sx / viewport.x * 2.0 - 1.0, 1.0 - sy / viewport.y * 2.0, 0.0, 1.0);
    o.uv = i.uv; o.col = i.col; o.world = i.pos;
    return o;
}
float4 PSSprite(PSIn i) : SV_TARGET {
    float4 t = tex0.Sample(samp0, i.uv);
    float3 lit = ambient.rgb;
    [loop] for (int k = 0; k < lightCount; k++) {
        float2 lp = lights[k * 2].xy;
        float radius = lights[k * 2].z;
        float inten = lights[k * 2].w;
        float3 lc = lights[k * 2 + 1].rgb;
        float d = distance(i.world, lp);
        float att = saturate(1.0 - d / max(radius, 1.0));
        att = att * att * inten;
        lit += lc * att;
    }
    float3 rgb = t.rgb * i.col.rgb * lit;
    return float4(rgb, t.a * i.col.a);
}
PSIn VSText(VSIn i) {
    PSIn o;
    o.pos = float4(i.pos.x / viewport.x * 2.0 - 1.0, 1.0 - i.pos.y / viewport.y * 2.0, 0.0, 1.0);
    o.uv = i.uv; o.col = i.col; o.world = float2(0.0, 0.0);
    return o;
}
float4 PSText(PSIn i) : SV_TARGET {
    float a = tex0.Sample(samp0, i.uv).r;
    if (a < 0.5) discard;
    return float4(i.col.rgb, 1.0);
}
)";

ComPtr<ID3DBlob> Compile(const char* entry, const char* target) {
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ComPtr<ID3DBlob> blob, err;
    HRESULT hr = D3DCompile(kHlsl, std::strlen(kHlsl), nullptr, nullptr, nullptr,
                            entry, target, flags, 0, &blob, &err);
    if (FAILED(hr)) {
        if (err) std::fprintf(stderr, "[hlsl %s] %s\n", entry, (const char*)err->GetBufferPointer());
        return nullptr;
    }
    return blob;
}

}

bool Renderer::init(void* hwnd, int width, int height, const std::string& assetsDir) {
    width_ = width; height_ = height;
    if (!createDevice()) return false;
    if (!createSwapChain(hwnd)) return false;
    if (!createTargets()) return false;
    if (!createSync()) return false;
    if (!createRootSignature()) return false;
    if (!createPipelines()) return false;
    if (!createBuffers()) return false;
    if (!loadResources(assetsDir)) return false;
    return true;
}

bool Renderer::createDevice() {
    UINT factoryFlags = 0;
#ifdef _DEBUG
    {
        ComPtr<ID3D12Debug> dbg;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) {
            dbg->EnableDebugLayer();
            factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif
    if (!CK(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory_)), "CreateDXGIFactory2")) return false;

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory_->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc{};
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_))))
            break;
        device_.Reset();
    }
    if (!device_) {
        ComPtr<IDXGIAdapter> warp;
        factory_->EnumWarpAdapter(IID_PPV_ARGS(&warp));
        if (!CK(D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)), "D3D12CreateDevice(WARP)"))
            return false;
    }

    D3D12_COMMAND_QUEUE_DESC qd{};
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    return CK(device_->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue_)), "CreateCommandQueue");
}

bool Renderer::createSwapChain(void* hwnd) {
    DXGI_SWAP_CHAIN_DESC1 sd{};
    sd.Width = width_; sd.Height = height_;
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = kFrameCount;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

    ComPtr<IDXGISwapChain1> sc1;
    if (!CK(factory_->CreateSwapChainForHwnd(queue_.Get(), (HWND)hwnd, &sd, nullptr, nullptr, &sc1),
            "CreateSwapChainForHwnd")) return false;
    factory_->MakeWindowAssociation((HWND)hwnd, DXGI_MWA_NO_ALT_ENTER);
    if (!CK(sc1.As(&swapChain_), "SwapChain.As")) return false;
    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
    return true;
}

bool Renderer::createTargets() {
    D3D12_DESCRIPTOR_HEAP_DESC hd{};
    hd.NumDescriptors = kFrameCount;
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    if (!CK(device_->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&rtvHeap_)), "CreateDescriptorHeap(RTV)")) return false;
    rtvDescSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < kFrameCount; ++i) {
        if (!CK(swapChain_->GetBuffer(i, IID_PPV_ARGS(&renderTargets_[i])), "GetBuffer")) return false;
        device_->CreateRenderTargetView(renderTargets_[i].Get(), nullptr, rtv);
        rtv.ptr += rtvDescSize_;
    }
    return true;
}

bool Renderer::createSync() {
    if (!CK(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator_)),
            "CreateCommandAllocator")) return false;
    if (!CK(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator_.Get(), nullptr,
            IID_PPV_ARGS(&cmd_)), "CreateCommandList")) return false;
    if (!CK(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)), "CreateFence")) return false;
    fenceValue_ = 0;
    fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    return fenceEvent_ != nullptr;
}

bool Renderer::createRootSignature() {
    D3D12_DESCRIPTOR_RANGE range{};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;
    range.RegisterSpace = 0;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER params[2]{};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0;
    params[0].Descriptor.RegisterSpace = 0;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges = &range;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC samp{};
    samp.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samp.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samp.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    samp.MaxLOD = D3D12_FLOAT32_MAX;
    samp.ShaderRegister = 0;
    samp.RegisterSpace = 0;
    samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = 2;
    desc.pParameters = params;
    desc.NumStaticSamplers = 1;
    desc.pStaticSamplers = &samp;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blob, err;
    if (!CK(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &err),
            "D3D12SerializeRootSignature")) {
        if (err) std::fprintf(stderr, "[rootsig] %s\n", (const char*)err->GetBufferPointer());
        return false;
    }
    return CK(device_->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
              IID_PPV_ARGS(&rootSig_)), "CreateRootSignature");
}

bool Renderer::createPipelines() {
    ComPtr<ID3DBlob> vsSprite = Compile("VSSprite", "vs_5_0");
    ComPtr<ID3DBlob> psSprite = Compile("PSSprite", "ps_5_0");
    ComPtr<ID3DBlob> vsText   = Compile("VSText", "vs_5_0");
    ComPtr<ID3DBlob> psText   = Compile("PSText", "ps_5_0");
    if (!vsSprite || !psSprite || !vsText || !psText) return false;

    D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_BLEND_DESC blend{};
    blend.RenderTarget[0].BlendEnable = TRUE;
    blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_RASTERIZER_DESC raster{};
    raster.FillMode = D3D12_FILL_MODE_SOLID;
    raster.CullMode = D3D12_CULL_MODE_NONE;
    raster.DepthClipEnable = TRUE;

    D3D12_DEPTH_STENCIL_DESC depth{};
    depth.DepthEnable = FALSE;
    depth.StencilEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
    pso.pRootSignature = rootSig_.Get();
    pso.InputLayout = { layout, _countof(layout) };
    pso.BlendState = blend;
    pso.RasterizerState = raster;
    pso.DepthStencilState = depth;
    pso.SampleMask = UINT_MAX;
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.SampleDesc.Count = 1;

    pso.VS = { vsSprite->GetBufferPointer(), vsSprite->GetBufferSize() };
    pso.PS = { psSprite->GetBufferPointer(), psSprite->GetBufferSize() };
    if (!CK(device_->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&spritePso_)), "CreatePSO(sprite)")) return false;

    pso.VS = { vsText->GetBufferPointer(), vsText->GetBufferSize() };
    pso.PS = { psText->GetBufferPointer(), psText->GetBufferSize() };
    if (!CK(device_->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&textPso_)), "CreatePSO(text)")) return false;

    return true;
}

bool Renderer::createBuffers() {
    D3D12_DESCRIPTOR_HEAP_DESC hd{};
    hd.NumDescriptors = (UINT)SpriteId::Count + 2;
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (!CK(device_->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&srvHeap_)), "CreateDescriptorHeap(SRV)")) return false;
    srvDescSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_HEAP_PROPERTIES upHeap = d3dutil::HeapProps(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RANGE noRead{ 0, 0 };

    UINT64 cbSize = Align(sizeof(SceneCB), 256);
    D3D12_RESOURCE_DESC cbDesc = d3dutil::BufferDesc(cbSize);
    if (!CK(device_->CreateCommittedResource(&upHeap, D3D12_HEAP_FLAG_NONE, &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cbuffer_)), "Create CB")) return false;
    if (!CK(cbuffer_->Map(0, &noRead, reinterpret_cast<void**>(&cbMapped_)), "Map CB")) return false;
    cbGpuBase_ = cbuffer_->GetGPUVirtualAddress();

    vbCapacityBytes_ = 6u * 1024u * 1024u;
    D3D12_RESOURCE_DESC vbDesc = d3dutil::BufferDesc(vbCapacityBytes_);
    if (!CK(device_->CreateCommittedResource(&upHeap, D3D12_HEAP_FLAG_NONE, &vbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vbuffer_)), "Create VB")) return false;
    if (!CK(vbuffer_->Map(0, &noRead, reinterpret_cast<void**>(&vbMapped_)), "Map VB")) return false;
    vbGpuBase_ = vbuffer_->GetGPUVirtualAddress();
    return true;
}

bool Renderer::loadResources(const std::string& assetsDir) {
    std::vector<ComPtr<ID3D12Resource>> uploads;
    UINT nextSlot = 0;

    if (!sprites_.loadAll(device_.Get(), cmd_.Get(), srvHeap_.Get(), srvDescSize_, nextSlot,
                          assetsDir, uploads))
        std::fprintf(stderr, "[renderer] some sprites failed to load (assetsDir=%s)\n", assetsDir.c_str());

    fontTex_ = CreateFontAtlas(device_.Get(), cmd_.Get(), srvHeap_.Get(), srvDescSize_, nextSlot,
                               fontGpu_, uploads);
    ++nextSlot;

    cmd_->Close();
    ID3D12CommandList* lists[] = { cmd_.Get() };
    queue_->ExecuteCommandLists(1, lists);
    flushGpu();
    return true;
}

void Renderer::flushGpu() {
    const UINT64 v = ++fenceValue_;
    queue_->Signal(fence_.Get(), v);
    if (fence_->GetCompletedValue() < v) {
        fence_->SetEventOnCompletion(v, (HANDLE)fenceEvent_);
        WaitForSingleObject((HANDLE)fenceEvent_, INFINITE);
    }
}

void Renderer::writeScene(const RenderFrame& frame) {
    SceneCB cb{};
    cb.camPos[0] = frame.camera.x;
    cb.camPos[1] = frame.camera.y;
    cb.zoom = frame.camera.zoom;
    cb.viewport[0] = (float)width_;
    cb.viewport[1] = (float)height_;
    cb.ambient[0] = frame.ambientR;
    cb.ambient[1] = frame.ambientG;
    cb.ambient[2] = frame.ambientB;
    cb.ambient[3] = 1.0f;

    int n = (int)std::min<size_t>(frame.lights.size(), kMaxLights);
    cb.lightCount = n;
    for (int i = 0; i < n; ++i) {
        const LightDraw& L = frame.lights[i];
        cb.lights[i * 2][0] = L.x; cb.lights[i * 2][1] = L.y;
        cb.lights[i * 2][2] = L.radius; cb.lights[i * 2][3] = L.intensity;
        cb.lights[i * 2 + 1][0] = L.r; cb.lights[i * 2 + 1][1] = L.g;
        cb.lights[i * 2 + 1][2] = L.b; cb.lights[i * 2 + 1][3] = 0.0f;
    }
    std::memcpy(cbMapped_, &cb, sizeof(cb));
}

void Renderer::draw(const RenderFrame& frame, int, int) {
    allocator_->Reset();
    cmd_->Reset(allocator_.Get(), nullptr);

    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();

    D3D12_RESOURCE_BARRIER toRT = d3dutil::Barrier(
        renderTargets_[frameIndex_].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmd_->ResourceBarrier(1, &toRT);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += SIZE_T(frameIndex_) * rtvDescSize_;
    cmd_->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    const float clear[4] = { 0.02f, 0.02f, 0.04f, 1.0f };
    cmd_->ClearRenderTargetView(rtv, clear, 0, nullptr);

    D3D12_VIEWPORT vp{ 0, 0, (float)width_, (float)height_, 0.0f, 1.0f };
    D3D12_RECT scissor{ 0, 0, (LONG)width_, (LONG)height_ };
    cmd_->RSSetViewports(1, &vp);
    cmd_->RSSetScissorRects(1, &scissor);

    ID3D12DescriptorHeap* heaps[] = { srvHeap_.Get() };
    cmd_->SetDescriptorHeaps(1, heaps);
    cmd_->SetGraphicsRootSignature(rootSig_.Get());

    writeScene(frame);
    cmd_->SetGraphicsRootConstantBufferView(0, cbGpuBase_);
    cmd_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    size_t vbOffset = 0;
    auto emit = [&](D3D12_GPU_DESCRIPTOR_HANDLE tex) {
        if (scratch_.empty()) return;
        size_t bytes = scratch_.size() * sizeof(float);
        if (vbOffset + bytes > vbCapacityBytes_) { scratch_.clear(); return; }
        std::memcpy(vbMapped_ + vbOffset, scratch_.data(), bytes);
        D3D12_VERTEX_BUFFER_VIEW vbv{ vbGpuBase_ + vbOffset, (UINT)bytes, kVertexStride };
        cmd_->IASetVertexBuffers(0, 1, &vbv);
        cmd_->SetGraphicsRootDescriptorTable(1, tex);
        cmd_->DrawInstanced((UINT)(scratch_.size() / 8), 1, 0, 0);
        vbOffset += bytes;
        scratch_.clear();
    };

    cmd_->SetPipelineState(spritePso_.Get());
    {
        SpriteId run = SpriteId::None;
        bool first = true;
        for (const SpriteDraw& s : frame.sprites) {
            if (s.id == SpriteId::None || !sprites_.has(s.id)) continue;
            if (!first && s.id != run) emit(sprites_.handle(run));
            run = s.id; first = false;

            float x0 = s.x, y0 = s.y, x1 = s.x + s.w, y1 = s.y + s.h;
            float r = s.r, g = s.g, b = s.b, a = s.a;
            auto push = [&](float px, float py, float u, float v) {
                scratch_.insert(scratch_.end(), { px, py, u, v, r, g, b, a });
            };
            push(x0, y0, 0, 0); push(x1, y0, 1, 0); push(x1, y1, 1, 1);
            push(x0, y0, 0, 0); push(x1, y1, 1, 1); push(x0, y1, 0, 1);
        }
        if (!first) emit(sprites_.handle(run));
    }

    cmd_->SetPipelineState(textPso_.Get());
    {
        const float uStep = 1.0f / 16.0f, vStep = 1.0f / 8.0f;
        const float cell = float(kGlyph * kTextScale);
        scratch_.clear();
        for (const TextDraw& t : frame.texts) {
            for (size_t i = 0; i < t.text.size(); ++i) {
                int c = (unsigned char)t.text[i] & 0x7f;
                if (c == ' ') continue;
                float u0 = (c % 16) * uStep, v0 = (c / 16) * vStep;
                float u1 = u0 + uStep, v1 = v0 + vStep;
                float x0 = t.x + (float)i * cell, y0 = (float)t.y;
                float x1 = x0 + cell, y1 = y0 + cell;
                float r = t.r, g = t.g, b = t.b;
                auto push = [&](float px, float py, float u, float v) {
                    scratch_.insert(scratch_.end(), { px, py, u, v, r, g, b, 1.0f });
                };
                push(x0, y0, u0, v0); push(x1, y0, u1, v0); push(x1, y1, u1, v1);
                push(x0, y0, u0, v0); push(x1, y1, u1, v1); push(x0, y1, u0, v1);
            }
        }
        emit(fontGpu_);
    }

    D3D12_RESOURCE_BARRIER toPresent = d3dutil::Barrier(
        renderTargets_[frameIndex_].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    cmd_->ResourceBarrier(1, &toPresent);

    cmd_->Close();
    ID3D12CommandList* lists[] = { cmd_.Get() };
    queue_->ExecuteCommandLists(1, lists);
    swapChain_->Present(1, 0);
    flushGpu();
}

void Renderer::shutdown() {
    if (device_) flushGpu();
    if (fenceEvent_) { CloseHandle((HANDLE)fenceEvent_); fenceEvent_ = nullptr; }
    if (cbuffer_ && cbMapped_) { cbuffer_->Unmap(0, nullptr); cbMapped_ = nullptr; }
    if (vbuffer_ && vbMapped_) { vbuffer_->Unmap(0, nullptr); vbMapped_ = nullptr; }
}
