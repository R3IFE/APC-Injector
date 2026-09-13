
#pragma once

#if defined _M_X64
typedef uint64_t uintx_t;
#elif defined _M_IX86
typedef uint32_t uintx_t;
#endif

#include <fstream>
inline void Log(const char* fmt, ...) {
    char text[4096] = { 0 };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);
    std::ofstream logfile("log.txt", std::ios::app);
    if (logfile.is_open()) logfile << text << std::endl;
}



namespace hooks {
    extern void InitH();
}

namespace d3d12hook {
  
    typedef HRESULT(STDMETHODCALLTYPE* PresentD3D12)(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags);
    extern PresentD3D12 oPresentD3D12;

    typedef void(STDMETHODCALLTYPE* ExecuteCommandListsFn)(ID3D12CommandQueue* _this, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
    extern ExecuteCommandListsFn oExecuteCommandListsD3D12;

    typedef void(STDMETHODCALLTYPE* RSSetViewportsFn)(ID3D12GraphicsCommandList* _this, UINT NumViewports, const D3D12_VIEWPORT* pViewports);
    extern RSSetViewportsFn oRSSetViewportsD3D12;

    typedef void(STDMETHODCALLTYPE* IASetVertexBuffersFn)(ID3D12GraphicsCommandList* _this, UINT StartSlot, UINT NumViews, const D3D12_VERTEX_BUFFER_VIEW* pViews);
    extern IASetVertexBuffersFn oIASetVertexBuffersD3D12;

    typedef void(STDMETHODCALLTYPE* DrawIndexedInstancedFn)(ID3D12GraphicsCommandList* _this, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
    extern DrawIndexedInstancedFn oDrawIndexedInstancedD3D12;

    typedef void(STDMETHODCALLTYPE* SetGraphicsRootConstantBufferViewFn)(ID3D12GraphicsCommandList* _this, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
    extern SetGraphicsRootConstantBufferViewFn oSetGraphicsRootConstantBufferViewD3D12;

    typedef void(STDMETHODCALLTYPE* SetGraphicsRootDescriptorTableFn)(ID3D12GraphicsCommandList* dCommandList, UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);
    extern SetGraphicsRootDescriptorTableFn oSetGraphicsRootDescriptorTableD3D12;

    typedef void(STDMETHODCALLTYPE* OMSetRenderTargetsFn)(ID3D12GraphicsCommandList* dCommandList, UINT NumRenderTargetDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE* pRenderTargetDescriptors, BOOL RTsSingleHandleToDescriptorRange, const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilDescriptor);
    extern OMSetRenderTargetsFn oOMSetRenderTargetsD3D12;

    typedef void(STDMETHODCALLTYPE* SetGraphicsRootSignatureFn)(ID3D12GraphicsCommandList* dCommandList, ID3D12RootSignature* pRootSignature);
    extern SetGraphicsRootSignatureFn oSetGraphicsRootSignatureD3D12;

    typedef void(STDMETHODCALLTYPE* ResetFn)(ID3D12GraphicsCommandList* _this, ID3D12CommandAllocator* pAllocator, ID3D12PipelineState* pInitialState);
    extern ResetFn oResetD3D12;

    typedef void(STDMETHODCALLTYPE* IASetIndexBufferFn)(ID3D12GraphicsCommandList* dCommandList, const D3D12_INDEX_BUFFER_VIEW* pView);
    extern IASetIndexBufferFn oIASetIndexBufferD3D12;

    typedef void(STDMETHODCALLTYPE* SetPipelineStateFn)(ID3D12GraphicsCommandList* _this, ID3D12PipelineState* pso);
    extern SetPipelineStateFn oSetPipelineStateD3D12;

    typedef void(STDMETHODCALLTYPE* IASetPrimitiveTopologyFn)(ID3D12GraphicsCommandList* _this, D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology);
    extern IASetPrimitiveTopologyFn oIASetPrimitiveTopologyD3D12;

    typedef void(STDMETHODCALLTYPE* ResolveQueryDataFn)(
        ID3D12GraphicsCommandList* self, ID3D12QueryHeap* pQueryHeap,
        D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries,
        ID3D12Resource* pDestinationBuffer, UINT64 AlignedDestinationBufferOffset);
    extern ResolveQueryDataFn oResolveQueryDataD3D12;

    typedef void(STDMETHODCALLTYPE* ExecuteIndirectFn)(
        ID3D12GraphicsCommandList* _this, ID3D12CommandSignature* pCommandSignature,
        UINT MaxCommandCount, ID3D12Resource* pArgumentBuffer, UINT64 ArgumentBufferOffset,
        ID3D12Resource* pCountBuffer, UINT64 CountBufferOffset);
    extern ExecuteIndirectFn oExecuteIndirectD3D12;

    typedef void(STDMETHODCALLTYPE* SetDescriptorHeapsFn)(ID3D12GraphicsCommandList* cmdList, UINT NumHeaps, ID3D12DescriptorHeap* const* ppHeaps);
    extern SetDescriptorHeapsFn oSetDescriptorHeapsD3D12;

   
    extern long __fastcall hookPresentD3D12(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags);
    extern void STDMETHODCALLTYPE hookExecuteCommandListsD3D12(ID3D12CommandQueue* _this, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
    extern void STDMETHODCALLTYPE hookRSSetViewportsD3D12(ID3D12GraphicsCommandList* _this, UINT NumViewports, const D3D12_VIEWPORT* pViewports);
    extern void STDMETHODCALLTYPE hookIASetVertexBuffersD3D12(ID3D12GraphicsCommandList* _this, UINT StartSlot, UINT NumViews, const D3D12_VERTEX_BUFFER_VIEW* pViews);
    extern void STDMETHODCALLTYPE hookDrawIndexedInstancedD3D12(ID3D12GraphicsCommandList* _this, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
    extern void STDMETHODCALLTYPE hookSetGraphicsRootConstantBufferViewD3D12(ID3D12GraphicsCommandList* _this, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
    extern void STDMETHODCALLTYPE hookSetDescriptorHeapsD3D12(ID3D12GraphicsCommandList* cmdList, UINT NumHeaps, ID3D12DescriptorHeap* const* ppHeaps);
    extern void STDMETHODCALLTYPE hookSetGraphicsRootDescriptorTableD3D12(ID3D12GraphicsCommandList* dCommandList, UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);
    extern void STDMETHODCALLTYPE hookOMSetRenderTargetsD3D12(ID3D12GraphicsCommandList* dCommandList, UINT NumRenderTargetDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE* pRenderTargetDescriptors, BOOL RTsSingleHandleToDescriptorRange, const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilDescriptor);
    extern void STDMETHODCALLTYPE hookResolveQueryDataD3D12(ID3D12GraphicsCommandList* self, ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* pDestinationBuffer, UINT64 AlignedDestinationBufferOffset);
    extern void STDMETHODCALLTYPE hookSetGraphicsRootSignatureD3D12(ID3D12GraphicsCommandList* dCommandList, ID3D12RootSignature* pRootSignature);
    extern void STDMETHODCALLTYPE hookResetD3D12(ID3D12GraphicsCommandList* _this, ID3D12CommandAllocator* pAllocator, ID3D12PipelineState* pInitialState);
    extern void STDMETHODCALLTYPE hookIASetIndexBufferD3D12(ID3D12GraphicsCommandList* dCommandList, const D3D12_INDEX_BUFFER_VIEW* pView);
    extern void STDMETHODCALLTYPE hookSetPipelineStateD3D12(ID3D12GraphicsCommandList* _this, ID3D12PipelineState* pso);
    extern void STDMETHODCALLTYPE hookIASetPrimitiveTopologyD3D12(ID3D12GraphicsCommandList* _this, D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology);
    extern void STDMETHODCALLTYPE hookExecuteIndirectD3D12(
        ID3D12GraphicsCommandList* _this, ID3D12CommandSignature* pCommandSignature,
        UINT MaxCommandCount, ID3D12Resource* pArgumentBuffer, UINT64 ArgumentBufferOffset,
        ID3D12Resource* pCountBuffer, UINT64 CountBufferOffset);

    extern void release();
    bool IsInitialized();
}


using Microsoft::WRL::ComPtr;

namespace hooks {
    constexpr size_t kRSSetViewportsIndex = 21;
    constexpr size_t kIASetVertexBuffersIndex = 44;
    constexpr size_t kDrawIndexedInstancedIndex = 13;
    constexpr size_t kSetGraphicsRootConstantBufferViewIndex = 38;
    constexpr size_t kSetDescriptorHeapsIndex = 28;
    constexpr size_t kSetGraphicsRootDescriptorTableIndex = 32;
    constexpr size_t kOMSetRenderTargetsIndex = 46;
    constexpr size_t kResolveQueryDataIndex = 54;
    constexpr size_t kExecuteIndirectIndex = 59;
    constexpr size_t kSetGraphicsRootSignatureIndex = 30;
    constexpr size_t kResetIndex = 10;
    constexpr size_t kSetPipelineStateIndex = 25;
    constexpr size_t kIASetPrimitiveTopologyIndex = 20;
    constexpr size_t kIASetIndexBufferIndex = 43;

    static ComPtr<IDXGISwapChain3>       pSwapChain = nullptr;
    static ComPtr<ID3D12Device>          pDevice = nullptr;
    static ComPtr<ID3D12CommandQueue>    pCommandQueue = nullptr;
    static ComPtr<ID3D12CommandAllocator> pCommandAllocator = nullptr;
    static ComPtr<ID3D12GraphicsCommandList> pCommandList = nullptr;
    static HWND                          hDummyWindow = nullptr;
    static const wchar_t* dummyClassName = L"DummyWndClass";

    static LPVOID pRSSetViewportsTarget = nullptr;
    static LPVOID pIASetVertexBuffersTarget = nullptr;
    static LPVOID pDrawIndexedInstancedTarget = nullptr;
    static LPVOID pSetGraphicsRootConstantBufferViewTarget = nullptr;
    static LPVOID pSetDescriptorHeapsTarget = nullptr;
    static LPVOID pSetGraphicsRootDescriptorTableTarget = nullptr;
    static LPVOID pOMSetRenderTargetsTarget = nullptr;
    static LPVOID pResolveQueryDataTarget = nullptr;
    static LPVOID pExecuteIndirectTarget = nullptr;
    static LPVOID pSetGraphicsRootSignatureTarget = nullptr;
    static LPVOID pResetTarget = nullptr;
    static LPVOID pIASetIndexBufferTarget = nullptr;
    static LPVOID pSetPipelineStateTarget = nullptr;
    static LPVOID pIASetPrimitiveTopologyTarget = nullptr;

    static void CleanupDummyObjects()
    {
        if (hDummyWindow) {
            DestroyWindow(hDummyWindow);
            hDummyWindow = nullptr;
        }
        UnregisterClassW(dummyClassName, GetModuleHandle(nullptr));
        pSwapChain.Reset();
        pDevice.Reset();
        pCommandQueue.Reset();
        pCommandAllocator.Reset();
        pCommandList.Reset();
    }

    static HRESULT CreateDeviceAndSwapChain() {
        WNDCLASSEXW wc = {sizeof(WNDCLASSEXW),CS_CLASSDC,DefWindowProcW,0,0,GetModuleHandleW(nullptr),nullptr,nullptr,nullptr,nullptr,dummyClassName,nullptr};
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            Log("[hooks] RegisterClassExW failed: %u\n", GetLastError());
            return E_FAIL;
        }

        hDummyWindow = CreateWindowExW(0, dummyClassName, L"Dummy",WS_OVERLAPPEDWINDOW,0,0,1,1,nullptr,nullptr,wc.hInstance,nullptr);
        if (!hDummyWindow) return E_FAIL;

        ComPtr<IDXGIFactory4> factory;
        HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
        if (FAILED(hr)) return hr;

        hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice));
        if (FAILED(hr)) return hr;

        D3D12_COMMAND_QUEUE_DESC cqDesc = {};
        cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        hr = pDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&pCommandQueue));
        if (FAILED(hr)) return hr;

        hr = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator));
        if (FAILED(hr)) return hr;

        hr = pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&pCommandList));
        if (FAILED(hr)) return hr;
        pCommandList->Close();

        DXGI_SWAP_CHAIN_DESC1 scDesc = {};
        scDesc.BufferCount = 2;
        scDesc.Width = 1;
        scDesc.Height = 1;
        scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        scDesc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swapChain1;
        hr = factory->CreateSwapChainForHwnd(pCommandQueue.Get(),hDummyWindow,&scDesc,nullptr,nullptr,&swapChain1);
        if (FAILED(hr)) return hr;

        hr = swapChain1.As(&pSwapChain);
        return hr;
    }

    static void** g_cmdListVtable = nullptr;

    static void* VtableSwap(void** vtable, size_t idx, void* newFn) {
        void* original = vtable[idx];
        DWORD old;
        VirtualProtect(&vtable[idx], sizeof(void*), PAGE_READWRITE, &old);
        vtable[idx] = newFn;
        VirtualProtect(&vtable[idx], sizeof(void*), old, &old);
        return original;
    }

    void InitH() {
        Log("d3d12.dll loaded at %p", GetModuleHandleA("d3d12.dll"));
        Log("D3D12Core.dll loaded at %p", GetModuleHandleA("D3D12Core.dll"));

        struct CleanupGuard { ~CleanupGuard() { CleanupDummyObjects(); } } cleanup;

        if (FAILED(CreateDeviceAndSwapChain())) {
            Log("[hooks] Failed to create dummy device/swapchain.\n");
            return;
        }

        if (!pCommandList) {
            Log("[hooks] CRITICAL: pCommandList is NULL!\n");
            return;
        }

        ComPtr<ID3D12GraphicsCommandList> baseCL;
        HRESULT hr = pCommandList.As(&baseCL);
        if (FAILED(hr)) { Log("Failed to QI ID3D12GraphicsCommandList\n"); return; }
        auto slVTable = *reinterpret_cast<void***>(baseCL.Get());
        g_cmdListVtable = slVTable;

        #define VHOOK(Idx, hookFn, orig) \
            orig = reinterpret_cast<decltype(orig)>(VtableSwap(slVTable, Idx, reinterpret_cast<void*>(hookFn)));

        VHOOK(kRSSetViewportsIndex,                    d3d12hook::hookRSSetViewportsD3D12,                    d3d12hook::oRSSetViewportsD3D12)
        VHOOK(kIASetVertexBuffersIndex,                d3d12hook::hookIASetVertexBuffersD3D12,                d3d12hook::oIASetVertexBuffersD3D12)
        VHOOK(kDrawIndexedInstancedIndex,              d3d12hook::hookDrawIndexedInstancedD3D12,              d3d12hook::oDrawIndexedInstancedD3D12)
        VHOOK(kSetGraphicsRootConstantBufferViewIndex, d3d12hook::hookSetGraphicsRootConstantBufferViewD3D12, d3d12hook::oSetGraphicsRootConstantBufferViewD3D12)
        VHOOK(kSetDescriptorHeapsIndex,                d3d12hook::hookSetDescriptorHeapsD3D12,                d3d12hook::oSetDescriptorHeapsD3D12)
        VHOOK(kSetGraphicsRootDescriptorTableIndex,    d3d12hook::hookSetGraphicsRootDescriptorTableD3D12,    d3d12hook::oSetGraphicsRootDescriptorTableD3D12)
        VHOOK(kOMSetRenderTargetsIndex,                d3d12hook::hookOMSetRenderTargetsD3D12,                d3d12hook::oOMSetRenderTargetsD3D12)
        VHOOK(kResolveQueryDataIndex,                  d3d12hook::hookResolveQueryDataD3D12,                  d3d12hook::oResolveQueryDataD3D12)
        VHOOK(kExecuteIndirectIndex,                   d3d12hook::hookExecuteIndirectD3D12,                   d3d12hook::oExecuteIndirectD3D12)
        VHOOK(kSetGraphicsRootSignatureIndex,          d3d12hook::hookSetGraphicsRootSignatureD3D12,          d3d12hook::oSetGraphicsRootSignatureD3D12)
        VHOOK(kResetIndex,                             d3d12hook::hookResetD3D12,                             d3d12hook::oResetD3D12)
        VHOOK(kIASetIndexBufferIndex,                  d3d12hook::hookIASetIndexBufferD3D12,                  d3d12hook::oIASetIndexBufferD3D12)
        VHOOK(kSetPipelineStateIndex,                  d3d12hook::hookSetPipelineStateD3D12,                  d3d12hook::oSetPipelineStateD3D12)
        VHOOK(kIASetPrimitiveTopologyIndex,            d3d12hook::hookIASetPrimitiveTopologyD3D12,            d3d12hook::oIASetPrimitiveTopologyD3D12)

        #undef VHOOK

        Log("[hooks] Vtable hooks installed.");
    }

    void Remove()
    {
        if (!g_cmdListVtable) return;

        #define VRESTORE(Idx, orig) \
            if (orig) { \
                DWORD old; \
                VirtualProtect(&g_cmdListVtable[Idx], sizeof(void*), PAGE_READWRITE, &old); \
                g_cmdListVtable[Idx] = reinterpret_cast<void*>(orig); \
                VirtualProtect(&g_cmdListVtable[Idx], sizeof(void*), old, &old); \
                orig = nullptr; \
            }

        VRESTORE(kRSSetViewportsIndex,                    d3d12hook::oRSSetViewportsD3D12)
        VRESTORE(kIASetVertexBuffersIndex,                d3d12hook::oIASetVertexBuffersD3D12)
        VRESTORE(kDrawIndexedInstancedIndex,              d3d12hook::oDrawIndexedInstancedD3D12)
        VRESTORE(kSetGraphicsRootConstantBufferViewIndex, d3d12hook::oSetGraphicsRootConstantBufferViewD3D12)
        VRESTORE(kSetDescriptorHeapsIndex,                d3d12hook::oSetDescriptorHeapsD3D12)
        VRESTORE(kSetGraphicsRootDescriptorTableIndex,    d3d12hook::oSetGraphicsRootDescriptorTableD3D12)
        VRESTORE(kOMSetRenderTargetsIndex,                d3d12hook::oOMSetRenderTargetsD3D12)
        VRESTORE(kResolveQueryDataIndex,                  d3d12hook::oResolveQueryDataD3D12)
        VRESTORE(kExecuteIndirectIndex,                   d3d12hook::oExecuteIndirectD3D12)
        VRESTORE(kSetGraphicsRootSignatureIndex,          d3d12hook::oSetGraphicsRootSignatureD3D12)
        VRESTORE(kResetIndex,                             d3d12hook::oResetD3D12)
        VRESTORE(kIASetIndexBufferIndex,                  d3d12hook::oIASetIndexBufferD3D12)
        VRESTORE(kSetPipelineStateIndex,                  d3d12hook::oSetPipelineStateD3D12)
        VRESTORE(kIASetPrimitiveTopologyIndex,            d3d12hook::oIASetPrimitiveTopologyD3D12)

        #undef VRESTORE

        g_cmdListVtable = nullptr;
        Log("[hooks] Vtable hooks removed.");
    }
}



namespace globals {
    extern HMODULE mainModule;
    extern HWND mainWindow;
    extern int openMenuKey;
    enum class Backend { None, DX12 };
    extern Backend activeBackend;
    extern Backend preferredBackend;
}

namespace globals {
    HMODULE mainModule = nullptr;
    HWND mainWindow = nullptr;
    int openMenuKey = VK_INSERT;
    Backend preferredBackend = Backend::DX12;
    Backend activeBackend = Backend::None;
}



struct ThreadDrawState {
    UINT StartSlot = 0;
    UINT Strides[16] = {};
    UINT numViews = 0;
    uint32_t StrideHash = 0;
    D3D12_VIEWPORT currentViewport = {};
    UINT numViewports = 0;
    UINT currentNumRTVs = 0;
    DXGI_FORMAT currentIndexFormat = DXGI_FORMAT_UNKNOWN;
    UINT currentGPUIAddress;
    D3D12_CPU_DESCRIPTOR_HANDLE currentDSVHandle = {};
    bool hasDSV = false;
    ID3D12PipelineState* currentPSO = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY currentTopology;
};

struct CommandListState {
    ID3D12GraphicsCommandList* cmdListPtr = nullptr;
    UINT lastRCBVindex = UINT_MAX;
    UINT lastRDTindex = UINT_MAX;
    D3D12_GPU_VIRTUAL_ADDRESS cbvGPUAddress[32] = {};
    ID3D12RootSignature* currentRootSig = nullptr;
};

struct PSOStats {
    ID3D12PipelineState* pso;
    UINT maxIndexCount;
};

struct PerThreadData {
    ThreadDrawState t;
    CommandListState cache;
    PSOStats psoStats[128] = {};
    UINT psoCount = 0;
    ID3D12GraphicsCommandList* tlsCmdList = nullptr;
    uint32_t tlsRootSigID = 0;
};

static DWORD g_tlsIndex = TLS_OUT_OF_INDEXES;

static void TlsInit() {
    g_tlsIndex = TlsAlloc();
}

static PerThreadData* GetPerThread() {
    auto* p = static_cast<PerThreadData*>(TlsGetValue(g_tlsIndex));
    if (!p) {
        p = static_cast<PerThreadData*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PerThreadData)));
        if (p) {
            p->cache.lastRCBVindex = UINT_MAX;
            p->cache.lastRDTindex = UINT_MAX;
        }
        TlsSetValue(g_tlsIndex, p);
    }
    return p;
}

#define t_ (GetPerThread()->t)
#define cache (GetPerThread()->cache)
#define psoStats (GetPerThread()->psoStats)
#define psoCount (GetPerThread()->psoCount)



uint32_t fastStrideHash(const uint32_t* data, size_t count) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < count; ++i) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash % 100;
}

#include <shared_mutex>
#include <unordered_map>

std::shared_mutex rootSigMutex;
uint32_t nextRuntimeSigID = 1;
std::unordered_map<ID3D12RootSignature*, uint32_t> rootSigToID;

#define tlsCurrentCmdList (GetPerThread()->tlsCmdList)
#define tlsCurrentRootSigID (GetPerThread()->tlsRootSigID)



#include <DirectXMath.h>

int coloroffset = 0;
ComPtr<ID3D12Device> pDevice = nullptr;
bool colorinitialized = false;
ComPtr<ID3D12Resource> g_pCustomConstantBuffer = nullptr;
UINT8* g_pMappedConstantBuffer = nullptr;
const UINT MAX_CB_SIZE = 1024 * 64;

struct alignas(256) MyMaterialConstants {
    DirectX::XMFLOAT4 color[16];
};

bool CreateColorConstantBuffer()
{
    if (!pDevice) return false;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = MAX_CB_SIZE;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = pDevice->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&g_pCustomConstantBuffer));
    if (FAILED(hr)) return false;

    D3D12_RANGE readRange = { 0, 0 };
    hr = g_pCustomConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&g_pMappedConstantBuffer));
    return SUCCEEDED(hr);
}

void CleanupColorBuffer()
{
    if (g_pCustomConstantBuffer) {
        if (g_pMappedConstantBuffer) {
            g_pCustomConstantBuffer->Unmap(0, nullptr);
            g_pMappedConstantBuffer = nullptr;
        }
        g_pCustomConstantBuffer.Reset();
    }
    colorinitialized = false;
}
