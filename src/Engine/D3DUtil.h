#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <d3d12.h>
#include <wrl/client.h>

namespace d3dutil {

using Microsoft::WRL::ComPtr;

inline D3D12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE type) {
    D3D12_HEAP_PROPERTIES h{};
    h.Type = type;
    h.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    h.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    h.CreationNodeMask = 1;
    h.VisibleNodeMask = 1;
    return h;
}

inline D3D12_RESOURCE_DESC BufferDesc(UINT64 bytes) {
    D3D12_RESOURCE_DESC d{};
    d.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    d.Width = bytes;
    d.Height = 1;
    d.DepthOrArraySize = 1;
    d.MipLevels = 1;
    d.Format = DXGI_FORMAT_UNKNOWN;
    d.SampleDesc.Count = 1;
    d.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    d.Flags = D3D12_RESOURCE_FLAG_NONE;
    return d;
}

inline D3D12_RESOURCE_BARRIER Barrier(ID3D12Resource* res,
    D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    b.Transition.pResource = res;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = before;
    b.Transition.StateAfter = after;
    return b;
}

inline ComPtr<ID3D12Resource> UploadTexture(
    ID3D12Device* dev, ID3D12GraphicsCommandList* cmd,
    UINT w, UINT h, DXGI_FORMAT fmt, UINT bytesPerPixel,
    const void* data, std::vector<ComPtr<ID3D12Resource>>& uploadsKeepAlive) {

    D3D12_RESOURCE_DESC td{};
    td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width = w;
    td.Height = h;
    td.DepthOrArraySize = 1;
    td.MipLevels = 1;
    td.Format = fmt;
    td.SampleDesc.Count = 1;
    td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    td.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES defHeap = HeapProps(D3D12_HEAP_TYPE_DEFAULT);
    ComPtr<ID3D12Resource> tex;
    dev->CreateCommittedResource(&defHeap, D3D12_HEAP_FLAG_NONE, &td,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&tex));

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};
    UINT numRows = 0;
    UINT64 rowSize = 0, totalBytes = 0;
    dev->GetCopyableFootprints(&td, 0, 1, 0, &fp, &numRows, &rowSize, &totalBytes);

    D3D12_HEAP_PROPERTIES upHeap = HeapProps(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC ub = BufferDesc(totalBytes);
    ComPtr<ID3D12Resource> up;
    dev->CreateCommittedResource(&upHeap, D3D12_HEAP_FLAG_NONE, &ub,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&up));

    uint8_t* mapped = nullptr;
    D3D12_RANGE noRead{ 0, 0 };
    up->Map(0, &noRead, reinterpret_cast<void**>(&mapped));
    const uint8_t* src = static_cast<const uint8_t*>(data);
    const size_t srcPitch = size_t(w) * bytesPerPixel;
    for (UINT row = 0; row < numRows; ++row)
        std::memcpy(mapped + fp.Offset + size_t(row) * fp.Footprint.RowPitch,
                    src + size_t(row) * srcPitch, size_t(rowSize));
    up->Unmap(0, nullptr);

    D3D12_TEXTURE_COPY_LOCATION dst{};
    dst.pResource = tex.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION srcLoc{};
    srcLoc.pResource = up.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.PlacedFootprint = fp;

    cmd->CopyTextureRegion(&dst, 0, 0, 0, &srcLoc, nullptr);

    D3D12_RESOURCE_BARRIER toSrv = Barrier(tex.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmd->ResourceBarrier(1, &toSrv);

    uploadsKeepAlive.push_back(up);
    return tex;
}

inline D3D12_GPU_DESCRIPTOR_HANDLE CreateSRVAt(
    ID3D12Device* dev, ID3D12DescriptorHeap* heap, UINT descSize, UINT slot,
    ID3D12Resource* res, DXGI_FORMAT fmt) {

    D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Format = fmt;
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipLevels = 1;

    D3D12_CPU_DESCRIPTOR_HANDLE cpu = heap->GetCPUDescriptorHandleForHeapStart();
    cpu.ptr += SIZE_T(slot) * descSize;
    dev->CreateShaderResourceView(res, &desc, cpu);

    D3D12_GPU_DESCRIPTOR_HANDLE gpu = heap->GetGPUDescriptorHandleForHeapStart();
    gpu.ptr += UINT64(slot) * descSize;
    return gpu;
}

}
