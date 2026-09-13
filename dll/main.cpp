

#pragma once
#include <windows.h>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cstdarg>

#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d12.h>

#include <wrl/client.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

#include "minhook/include/MinHook.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")

#include "rtexplorer.h"
#include "main.h"
#include "config.h"
#include "overlay.h"



namespace hooks { void Remove(); }

namespace d3d12hook {
    PresentD3D12            oPresentD3D12 = nullptr;
    ExecuteCommandListsFn   oExecuteCommandListsD3D12 = nullptr;
    RSSetViewportsFn        oRSSetViewportsD3D12 = nullptr;
    IASetVertexBuffersFn    oIASetVertexBuffersD3D12 = nullptr;
    DrawIndexedInstancedFn  oDrawIndexedInstancedD3D12 = nullptr;
    SetGraphicsRootConstantBufferViewFn oSetGraphicsRootConstantBufferViewD3D12 = nullptr;
    SetDescriptorHeapsFn    oSetDescriptorHeapsD3D12 = nullptr;
    SetGraphicsRootDescriptorTableFn oSetGraphicsRootDescriptorTableD3D12 = nullptr;
    OMSetRenderTargetsFn    oOMSetRenderTargetsD3D12 = nullptr;
    ResolveQueryDataFn      oResolveQueryDataD3D12 = nullptr;
    ExecuteIndirectFn       oExecuteIndirectD3D12 = nullptr;
    SetGraphicsRootSignatureFn oSetGraphicsRootSignatureD3D12 = nullptr;
    ResetFn                 oResetD3D12 = nullptr;
    IASetIndexBufferFn      oIASetIndexBufferD3D12 = nullptr;
    SetPipelineStateFn      oSetPipelineStateD3D12 = nullptr;
    IASetPrimitiveTopologyFn oIASetPrimitiveTopologyD3D12 = nullptr;

    static ID3D12Device*            gDevice = nullptr;
    static ID3D12CommandQueue*      gCommandQueue = nullptr;
    static ID3D12DescriptorHeap*    gHeapRTV = nullptr;
    static ID3D12DescriptorHeap*    gHeapSRV = nullptr;
    static ID3D12GraphicsCommandList* gCommandList = nullptr;
    static ID3D12Fence*            gOverlayFence = nullptr;
    static HANDLE                  gFenceEvent = nullptr;
    static UINT64                  gOverlayFenceValue = 0;
    static uintx_t                 gBufferCount = 0;

    struct FrameContext {
        ID3D12CommandAllocator* allocator;
        ID3D12Resource* renderTarget;
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
        UINT64 FenceValue;
    };
    static FrameContext*          gFrameContexts = nullptr;
    static bool                   gInitialized = false;
    static bool                   gShutdown = false;
    static bool                   gAfterFirstPresent = false;

    void release();

   

    HRESULT WINAPI hookPresentD3D12(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags) {
        return oPresentD3D12(pSwapChain, SyncInterval, Flags);
    }

    void STDMETHODCALLTYPE hookExecuteCommandListsD3D12(ID3D12CommandQueue* _this, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists) {
        return oExecuteCommandListsD3D12(_this, NumCommandLists, ppCommandLists);
    }

    void STDMETHODCALLTYPE hookSetPipelineStateD3D12(ID3D12GraphicsCommandList* _this, ID3D12PipelineState* pso) {
        t_.currentPSO = pso;
        return oSetPipelineStateD3D12(_this, pso);
    }

    void STDMETHODCALLTYPE hookIASetPrimitiveTopologyD3D12(ID3D12GraphicsCommandList* _this, D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology) {
        t_.currentTopology = PrimitiveTopology;
        return oIASetPrimitiveTopologyD3D12(_this, PrimitiveTopology);
    }

    void STDMETHODCALLTYPE hookRSSetViewportsD3D12(ID3D12GraphicsCommandList* _this, UINT NumViewports, const D3D12_VIEWPORT* pViewports) {
        gInitialized = true;
        if (!pViewports || NumViewports == 0) { oRSSetViewportsD3D12(_this, NumViewports, pViewports); return; }
        if (NumViewports > 0 && pViewports) {
            t_.currentViewport = pViewports[0];
            t_.numViewports = NumViewports;
        }
        oRSSetViewportsD3D12(_this, NumViewports, pViewports);
    }

    void STDMETHODCALLTYPE hookIASetVertexBuffersD3D12(ID3D12GraphicsCommandList* _this, UINT StartSlot, UINT NumViews, const D3D12_VERTEX_BUFFER_VIEW* pViews) {
        t_.StartSlot = StartSlot;
        t_.numViews = NumViews;
        for (int i = 0; i < 16; ++i) t_.Strides[i] = 0;

        if (NumViews > 0 && pViews) {
            uint32_t strideData[16] = {};
            UINT validCount = (NumViews > 16) ? 16 : NumViews;
            for (UINT i = 0; i < validCount; ++i) {
                if (pViews[i].BufferLocation != 0 && pViews[i].StrideInBytes > 0 && pViews[i].StrideInBytes <= 200) {
                    t_.Strides[i] = pViews[i].StrideInBytes;
                    strideData[i] = pViews[i].StrideInBytes;
                }
            }
            t_.StrideHash = fastStrideHash(strideData, validCount);
        } else {
            t_.StrideHash = 0;
        }
        return oIASetVertexBuffersD3D12(_this, StartSlot, NumViews, pViews);
    }

    void STDMETHODCALLTYPE hookIASetIndexBufferD3D12(ID3D12GraphicsCommandList* dCommandList, const D3D12_INDEX_BUFFER_VIEW* pView) {
        if (pView != nullptr) {
            t_.currentIndexFormat = pView->Format;
            t_.currentGPUIAddress = pView->BufferLocation;
        }
        return oIASetIndexBufferD3D12(dCommandList, pView);
    }

    void STDMETHODCALLTYPE hookOMSetRenderTargetsD3D12(
        ID3D12GraphicsCommandList* dCommandList, UINT NumRenderTargetDescriptors,
        const D3D12_CPU_DESCRIPTOR_HANDLE* pRenderTargetDescriptors,
        BOOL RTsSingleHandleToDescriptorRange,
        const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilDescriptor) {

        if (dCommandList->GetType() == D3D12_COMMAND_LIST_TYPE_DIRECT) {
            t_.currentNumRTVs = NumRenderTargetDescriptors;
            if (pDepthStencilDescriptor != nullptr && pDepthStencilDescriptor->ptr != 0) {
                t_.currentDSVHandle = *pDepthStencilDescriptor;
                t_.hasDSV = true;
            } else {
                t_.hasDSV = false;
                t_.currentDSVHandle.ptr = 0;
            }
        }
        return oOMSetRenderTargetsD3D12(dCommandList, NumRenderTargetDescriptors, pRenderTargetDescriptors, RTsSingleHandleToDescriptorRange, pDepthStencilDescriptor);
    }

    void STDMETHODCALLTYPE hookDrawIndexedInstancedD3D12(ID3D12GraphicsCommandList* _this, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
    {
        if (!_this || InstanceCount == 0 || InstanceCount > 5 || IndexCountPerInstance < 120)
            return oDrawIndexedInstancedD3D12(_this, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

        if (_this->GetType() != D3D12_COMMAND_LIST_TYPE_DIRECT)
            return oDrawIndexedInstancedD3D12(_this, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

        if (t_.currentNumRTVs == 999 || !t_.hasDSV)
            return oDrawIndexedInstancedD3D12(_this, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

        if (t_.currentViewport.Width < 500.0f)
            return oDrawIndexedInstancedD3D12(_this, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

        if (t_.currentTopology != D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
            return oDrawIndexedInstancedD3D12(_this, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

        if (!t_.currentPSO)
            return oDrawIndexedInstancedD3D12(_this, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

        ID3D12PipelineState* currentPSO = t_.currentPSO;

        
        PSOStats* stats = nullptr;
        for (UINT i = 0; i < psoCount; ++i) {
            if (psoStats[i].pso == currentPSO) {
                stats = &psoStats[i];
                if (i > 0) { PSOStats tmp = psoStats[i]; psoStats[i] = psoStats[0]; psoStats[0] = tmp; stats = &psoStats[0]; }
                break;
            }
        }
        if (!stats && psoCount < _countof(psoStats)) {
            stats = &psoStats[psoCount++];
            stats->pso = currentPSO;
            stats->maxIndexCount = 0;
        }
        if (stats) {
            if (IndexCountPerInstance > stats->maxIndexCount) stats->maxIndexCount = IndexCountPerInstance;
            if (stats->maxIndexCount < 300)
                oDrawIndexedInstancedD3D12(_this, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        }

        
        if (g_rtCollect && !g_rtFreeze) {
            RTRegisterDraw(currentPSO, (uint64_t)t_.currentGPUIAddress, IndexCountPerInstance,
                t_.StrideHash, t_.StartSlot, (uint32_t)t_.currentIndexFormat, t_.currentNumRTVs,
                0, (uint32_t)t_.currentTopology, t_.numViews, (uint32_t)t_.currentViewport.Width,
                (uint32_t)t_.currentViewport.Height, t_.hasDSV, false);
        }

       
        bool rtMatch = g_rtWallhackOn && RTIsWallhackEnabled(currentPSO);
        if (rtMatch) {
            if (g_rtHideMode) return;
            const D3D12_VIEWPORT originalVp = t_.currentViewport;
            if (originalVp.Width >= 128 && originalVp.Width < 16384) {
                D3D12_VIEWPORT hVp = originalVp;
                hVp.MinDepth = reversedDepth ? 0.0f : 0.9f;
                hVp.MaxDepth = reversedDepth ? 0.1f : 1.0f;
                _this->RSSetViewports(1, &hVp);
                oDrawIndexedInstancedD3D12(_this, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
                _this->RSSetViewports(1, &originalVp);
                return;
            }
        }

        
        const UINT currentStrides = t_.StrideHash + t_.StartSlot;
        bool isModelDraw = false;

        if (currentStrides == countstride1 || currentStrides == countstride2 || currentStrides == countstride3 || currentStrides == countstride4 || currentStrides == countstride5 ||
            cache.lastRDTindex == countGRootDescriptor || cache.lastRCBVindex == countGRootConstantBuffer ||
            t_.currentNumRTVs == countfindrendertarget || IndexCountPerInstance / 1000 == countIndexCount)
            isModelDraw = true;

        if (enabletemporaryids) {
            uint32_t rootSigID = (tlsCurrentCmdList == _this) ? tlsCurrentRootSigID : 0;
            if (rootSigID == countcurrentRootSigID || rootSigID == countcurrentRootSigID2)
                isModelDraw = true;
            else {
                uint8_t indexid = static_cast<uint8_t>((t_.currentGPUIAddress >> 12) % 100);
                if (indexid == countcurrentIndexAddress || indexid == countcurrentIndexAddress2 || indexid == countcurrentIndexAddress3)
                    isModelDraw = true;
                uint8_t psoid = static_cast<uint8_t>(((UINT)currentPSO >> 12) % 100);
                if (psoid == countcurrentPSOAddress)
                    isModelDraw = true;
            }
        }

        if (isModelDraw) {
            if (enablefilters) {
                if (filterindexformat && t_.currentIndexFormat != countfilterindexformat) goto skip;
                if (filternumViews && t_.numViews + t_.numViewports != countfilternumViews) goto skip;
                if (filterrendertarget && (t_.currentNumRTVs != countfilterrendertarget && t_.currentNumRTVs != countfilterrendertarget2)) goto skip;
                if (filterGRootDescriptor && (cache.lastRDTindex != countfilterGRootDescriptor && cache.lastRDTindex != countfilterGRootDescriptor2 && cache.lastRDTindex != countfilterGRootDescriptor3)) goto skip;
                if (filterGRootConstantBuffer && (cache.lastRCBVindex != countfilterGRootConstantBuffer && cache.lastRCBVindex != countfilterGRootConstantBuffer2 && cache.lastRCBVindex != countfilterGRootConstantBuffer3)) goto skip;
                if (filterIndexCountPerInstance && (IndexCountPerInstance / 1000 != countfilterIndexCountPerInstance && IndexCountPerInstance / 1000 != countfilterIndexCountPerInstance2)) goto skip;
            }
            if (enableignores) {
                if (ignorenumViews && t_.numViews + t_.numViewports == countignorenumViews) goto skip;
                if (ignorerendertarget && t_.currentNumRTVs == countignorerendertarget) goto skip;
                if (ignoreGRootDescriptor && (cache.lastRDTindex == countignoreGRootDescriptor || cache.lastRDTindex == countignoreGRootDescriptor2 || cache.lastRDTindex == countignoreGRootDescriptor3)) goto skip;
                if (ignoreGRootConstantBuffer && (cache.lastRCBVindex == countignoreGRootConstantBuffer || cache.lastRCBVindex == countignoreGRootConstantBuffer2 || cache.lastRCBVindex == countignoreGRootConstantBuffer3)) goto skip;
                if (ignoreIndexCountPerInstance && IndexCountPerInstance/1000 <= countignoreIndexCountPerInstance) goto skip;
            }

            if (enablecolor) {
                if (!colorinitialized) {
                    HRESULT hr = _this->GetDevice(__uuidof(ID3D12Device), (void**)&pDevice);
                    if (FAILED(hr)) return oDrawIndexedInstancedD3D12(_this, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
                    if (CreateColorConstantBuffer()) colorinitialized = true;
                    else Log("Failed to create constant buffer for colors");
                }
                if(cache.lastRCBVindex==0 && cri0==1|| cache.lastRCBVindex == 1 && cri1 == 1|| cache.lastRCBVindex == 2 && cri2 == 1 || cache.lastRCBVindex == 3 && cri3 == 1||
                    cache.lastRCBVindex == 4 && cri4 == 1 || cache.lastRCBVindex == 5 && cri5 == 1 || cache.lastRCBVindex == 6 && cri6 == 1 || cache.lastRCBVindex == 7 && cri7 == 1||
                    cache.lastRCBVindex == 8 && cri8 == 1 || cache.lastRCBVindex == 9 && cri9 == 1 || cache.lastRCBVindex == 10 && cri10 == 1) {
                    if (colorinitialized && g_pMappedConstantBuffer) {
                        if (coloroffset < 0) coloroffset = 0;
                        if (coloroffset > (MAX_CB_SIZE - 256)) coloroffset = MAX_CB_SIZE - 256;
                        DirectX::XMFLOAT4 myColor = { 15.0f, 0.0f, 15.0f, 1.0f };
                        auto* constants = reinterpret_cast<MyMaterialConstants*>(g_pMappedConstantBuffer + coloroffset);
                        for (int i = 0; i < 16; i++) constants->color[i] = myColor;
                        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = g_pCustomConstantBuffer->GetGPUVirtualAddress() + coloroffset;
                        _this->SetGraphicsRootConstantBufferView(cache.lastRCBVindex, cbAddr);
                    }
                }
            }

            const D3D12_VIEWPORT originalVp = t_.currentViewport;
            if (originalVp.Width >= 128 && originalVp.Width < 16384) {
                D3D12_VIEWPORT hVp = originalVp;
                hVp.MinDepth = reversedDepth ? 0.0f : 0.9f;
                hVp.MaxDepth = reversedDepth ? 0.1f : 1.0f;
                _this->RSSetViewports(1, &hVp);
                oDrawIndexedInstancedD3D12(_this, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
                _this->RSSetViewports(1, &originalVp);
                return;
            }
        }

    skip:
        oDrawIndexedInstancedD3D12(_this, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    void STDMETHODCALLTYPE hookExecuteIndirectD3D12(ID3D12GraphicsCommandList* _this, ID3D12CommandSignature* pCommandSignature, UINT MaxCommandCount, ID3D12Resource* pArgumentBuffer,
        UINT64 ArgumentBufferOffset, ID3D12Resource* pCountBuffer, UINT64 CountBufferOffset)
    {
        if (!_this || MaxCommandCount == 0)
            return oExecuteIndirectD3D12(_this, pCommandSignature, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset, pCountBuffer, CountBufferOffset);

        if (_this->GetType() != D3D12_COMMAND_LIST_TYPE_DIRECT)
            return oExecuteIndirectD3D12(_this, pCommandSignature, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset, pCountBuffer, CountBufferOffset);

        
        if (g_asEnabled && g_asSkipInterval >= 0 &&
            RTHasDiscriminator(g_asMinCmd, g_asMaxCmd, g_asRequireCount, g_asStride, g_asNumRTVs, g_asDrawType, g_asMinVPWidth, g_asMaxVPWidth) &&
            RTFilterMatch(g_asMinCmd, g_asMaxCmd, g_asRequireCount, g_asStride, g_asNumRTVs, g_asDrawType, g_asMinVPWidth, g_asMaxVPWidth,
                MaxCommandCount, pCountBuffer != nullptr, t_.StrideHash, t_.currentNumRTVs, (uint32_t)t_.currentTopology, t_.currentViewport.Width))
        {
            if (RTAutoSkipShouldSkip(g_asSkipInterval)) {
                g_asSkippedThisFrame.fetch_add(1, std::memory_order_relaxed);
                return;
            }
        }

     
        if (t_.currentNumRTVs == 999 || !t_.hasDSV || t_.currentViewport.Width < 500.0f)
            return oExecuteIndirectD3D12(_this, pCommandSignature, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset, pCountBuffer, CountBufferOffset);

        if (g_eiMaxCmdCount > 0 && MaxCommandCount > (UINT)g_eiMaxCmdCount)
            return oExecuteIndirectD3D12(_this, pCommandSignature, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset, pCountBuffer, CountBufferOffset);

        if (t_.currentTopology != D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
            return oExecuteIndirectD3D12(_this, pCommandSignature, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset, pCountBuffer, CountBufferOffset);

        if (!t_.currentPSO)
            return oExecuteIndirectD3D12(_this, pCommandSignature, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset, pCountBuffer, CountBufferOffset);

        PSOStats* stats = nullptr;
        for (UINT i = 0; i < psoCount; ++i) {
            if (psoStats[i].pso == t_.currentPSO) { stats = &psoStats[i]; break; }
        }
        if (stats && stats->maxIndexCount > 0 && stats->maxIndexCount < 100)
            return oExecuteIndirectD3D12(_this, pCommandSignature, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset, pCountBuffer, CountBufferOffset);

        ExecuteIndirectisCalled = 1;

       
        if (g_rtCollect && !g_rtFreeze) {
            RTRegisterDraw(t_.currentPSO, (uint64_t)t_.currentGPUIAddress, MaxCommandCount,
                t_.StrideHash, t_.StartSlot, (uint32_t)t_.currentIndexFormat, t_.currentNumRTVs,
                MaxCommandCount, (uint32_t)t_.currentTopology, t_.numViews, (uint32_t)t_.currentViewport.Width,
                (uint32_t)t_.currentViewport.Height, t_.hasDSV, true);
        }

       
        bool rtMatch = g_rtWallhackOn && RTIsWallhackEnabled(t_.currentPSO);
        if (rtMatch) {
            if (g_rtHideMode) return;
            const D3D12_VIEWPORT originalVp = t_.currentViewport;
            if (originalVp.Width >= 128 && originalVp.Width < 16384) {
                D3D12_VIEWPORT hVp = originalVp;
                hVp.MinDepth = reversedDepth ? 0.0f : 0.9f;
                hVp.MaxDepth = reversedDepth ? 0.1f : 1.0f;
                _this->RSSetViewports(1, &hVp);
                oExecuteIndirectD3D12(_this, pCommandSignature, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset, pCountBuffer, CountBufferOffset);
                _this->RSSetViewports(1, &originalVp);
                return;
            }
        }

       
        const UINT currentExIStride = t_.StrideHash + t_.StartSlot;
        bool isModelDraw = false;

        if (currentExIStride == countExIStride1 || currentExIStride == countExIStride2 || currentExIStride == countExIStride3 || currentExIStride == countExIStride4 ||
            cache.lastRDTindex == countGRootDescriptor || cache.lastRCBVindex == countGRootConstantBuffer ||
            t_.currentNumRTVs == countfindrendertarget)
            isModelDraw = true;

        if (enabletemporaryids) {
            uint32_t rootSigID = (tlsCurrentCmdList == _this) ? tlsCurrentRootSigID : 0;
            if (rootSigID == countcurrentRootSigID || rootSigID == countcurrentRootSigID2)
                isModelDraw = true;
            else {
                uint8_t indexid = static_cast<uint8_t>((t_.currentGPUIAddress >> 12) % 100);
                if (indexid == countcurrentIndexAddress || indexid == countcurrentIndexAddress2 || indexid == countcurrentIndexAddress3)
                    isModelDraw = true;
            }
        }

        if (isModelDraw) {
            if (enablefilters) {
                if (filterindexformat && t_.currentIndexFormat != countfilterindexformat) goto skip;
                if (filternumViews && t_.numViews + t_.numViewports != countfilternumViews) goto skip;
                if (filterrendertarget && (t_.currentNumRTVs != countfilterrendertarget && t_.currentNumRTVs != countfilterrendertarget2)) goto skip;
                if (filterGRootDescriptor && (cache.lastRDTindex != countfilterGRootDescriptor && cache.lastRDTindex != countfilterGRootDescriptor2 && cache.lastRDTindex != countfilterGRootDescriptor3)) goto skip;
                if (filterGRootConstantBuffer && (cache.lastRCBVindex != countfilterGRootConstantBuffer && cache.lastRCBVindex != countfilterGRootConstantBuffer2 && cache.lastRCBVindex != countfilterGRootConstantBuffer3)) goto skip;
            }
            if (enableignores) {
                if (ignorenumViews && t_.numViews + t_.numViewports == countignorenumViews) goto skip;
                if (ignorerendertarget && t_.currentNumRTVs == countignorerendertarget) goto skip;
                if (ignoreGRootDescriptor && (cache.lastRDTindex == countignoreGRootDescriptor || cache.lastRDTindex == countignoreGRootDescriptor2 || cache.lastRDTindex == countignoreGRootDescriptor3)) goto skip;
                if (ignoreGRootConstantBuffer && (cache.lastRCBVindex == countignoreGRootConstantBuffer || cache.lastRCBVindex == countignoreGRootConstantBuffer2 || cache.lastRCBVindex == countignoreGRootConstantBuffer3)) goto skip;
            }

            if (enablecolor) {
                if (!colorinitialized) {
                    HRESULT hr = _this->GetDevice(__uuidof(ID3D12Device), (void**)&pDevice);
                    if (FAILED(hr)) return oExecuteIndirectD3D12(_this, pCommandSignature, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset, pCountBuffer, CountBufferOffset);
                    if (CreateColorConstantBuffer()) colorinitialized = true;
                    else Log("Failed to create constant buffer for colors");
                }
                if (cache.lastRCBVindex == 0 && cri0 == 1 || cache.lastRCBVindex == 1 && cri1 == 1 || cache.lastRCBVindex == 2 && cri2 == 1 || cache.lastRCBVindex == 3 && cri3 == 1 ||
                    cache.lastRCBVindex == 4 && cri4 == 1 || cache.lastRCBVindex == 5 && cri5 == 1 || cache.lastRCBVindex == 6 && cri6 == 1 || cache.lastRCBVindex == 7 && cri7 == 1 ||
                    cache.lastRCBVindex == 8 && cri8 == 1 || cache.lastRCBVindex == 9 && cri9 == 1 || cache.lastRCBVindex == 10 && cri10 == 1) {
                    if (colorinitialized && g_pMappedConstantBuffer) {
                        if (coloroffset < 0) coloroffset = 0;
                        if (coloroffset > (MAX_CB_SIZE - 256)) coloroffset = MAX_CB_SIZE - 256;
                        DirectX::XMFLOAT4 myColor = { 15.0f, 0.0f, 15.0f, 1.0f };
                        auto* constants = reinterpret_cast<MyMaterialConstants*>(g_pMappedConstantBuffer + coloroffset);
                        for (int i = 0; i < 16; i++) constants->color[i] = myColor;
                        D3D12_GPU_VIRTUAL_ADDRESS cbAddr = g_pCustomConstantBuffer->GetGPUVirtualAddress() + coloroffset;
                        _this->SetGraphicsRootConstantBufferView(cache.lastRCBVindex, cbAddr);
                    }
                }
            }

            const D3D12_VIEWPORT originalVp = t_.currentViewport;
            if (originalVp.Width >= 128 && originalVp.Width < 16384) {
                D3D12_VIEWPORT hVp = originalVp;
                hVp.MinDepth = reversedDepth ? 0.0f : 0.9f;
                hVp.MaxDepth = reversedDepth ? 0.1f : 1.0f;
                _this->RSSetViewports(1, &hVp);
                oExecuteIndirectD3D12(_this, pCommandSignature, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset, pCountBuffer, CountBufferOffset);
                _this->RSSetViewports(1, &originalVp);
                return;
            }
        }

    skip:
        oExecuteIndirectD3D12(_this, pCommandSignature, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset, pCountBuffer, CountBufferOffset);
    }

    void STDMETHODCALLTYPE hookSetGraphicsRootConstantBufferViewD3D12(ID3D12GraphicsCommandList* _this, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
        if (cache.cmdListPtr != _this) { cache.cmdListPtr = _this; cache.lastRCBVindex = UINT_MAX; }
        if (BufferLocation != 0) { cache.lastRCBVindex = RootParameterIndex; cache.cbvGPUAddress[RootParameterIndex] = BufferLocation; }
        return oSetGraphicsRootConstantBufferViewD3D12(_this, RootParameterIndex, BufferLocation);
    }

    void STDMETHODCALLTYPE hookSetGraphicsRootDescriptorTableD3D12(ID3D12GraphicsCommandList* dCommandList, UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) {
        if (cache.cmdListPtr != dCommandList) { cache.cmdListPtr = dCommandList; cache.lastRDTindex = UINT_MAX; }
        if (BaseDescriptor.ptr != 0) { if (cache.lastRDTindex != RootParameterIndex) cache.lastRDTindex = RootParameterIndex; }
        return oSetGraphicsRootDescriptorTableD3D12(dCommandList, RootParameterIndex, BaseDescriptor);
    }

    void STDMETHODCALLTYPE hookSetGraphicsRootSignatureD3D12(ID3D12GraphicsCommandList* dCommandList, ID3D12RootSignature* pRootSignature) {
        if (!dCommandList || !pRootSignature)
            return oSetGraphicsRootSignatureD3D12(dCommandList, pRootSignature);

        cache.cmdListPtr = dCommandList;
        cache.lastRDTindex = UINT_MAX;
        cache.lastRCBVindex = UINT_MAX;
        cache.currentRootSig = pRootSignature;

        if ((countcurrentRootSigID > 0 || countcurrentRootSigID2 > 0) && (dCommandList && pRootSignature)) {
            uint32_t idToStore = 0;
            { std::shared_lock<std::shared_mutex> lock(rootSigMutex); auto it = rootSigToID.find(pRootSignature); if (it != rootSigToID.end()) idToStore = it->second; }
            if (idToStore == 0) {
                std::unique_lock<std::shared_mutex> lock(rootSigMutex);
                if (rootSigToID.find(pRootSignature) == rootSigToID.end()) { idToStore = nextRuntimeSigID++; rootSigToID[pRootSignature] = idToStore; }
                else idToStore = rootSigToID[pRootSignature];
            }
            tlsCurrentCmdList = dCommandList;
            tlsCurrentRootSigID = idToStore;
        }
        return oSetGraphicsRootSignatureD3D12(dCommandList, pRootSignature);
    }

    void STDMETHODCALLTYPE hookResetD3D12(ID3D12GraphicsCommandList* _this, ID3D12CommandAllocator* pAllocator, ID3D12PipelineState* pInitialState) {
        cache.cmdListPtr = _this;
        cache.lastRCBVindex = UINT_MAX;
        cache.lastRDTindex = UINT_MAX;
        return oResetD3D12(_this, pAllocator, pInitialState);
    }

    void STDMETHODCALLTYPE hookSetDescriptorHeapsD3D12(ID3D12GraphicsCommandList* cmdList, UINT NumHeaps, ID3D12DescriptorHeap* const* ppHeaps) {
        oSetDescriptorHeapsD3D12(cmdList, NumHeaps, ppHeaps);
    }

    void STDMETHODCALLTYPE hookResolveQueryDataD3D12(
        ID3D12GraphicsCommandList* self, ID3D12QueryHeap* pQueryHeap,
        D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries,
        ID3D12Resource* pDestinationBuffer, UINT64 AlignedDestinationBufferOffset)
    {
        if (DisableOcclusionCulling && (Type == D3D12_QUERY_TYPE_OCCLUSION || Type == D3D12_QUERY_TYPE_BINARY_OCCLUSION)) {
            if (self->GetType() != D3D12_COMMAND_LIST_TYPE_BUNDLE) {
                ID3D12GraphicsCommandList2* cl2 = nullptr;
                if (SUCCEEDED(self->QueryInterface(IID_PPV_ARGS(&cl2)))) {
                    UINT64 visibleValue = (Type == D3D12_QUERY_TYPE_OCCLUSION) ? 0xFFFFFFFFull : 1ull;
                    const UINT batchSize = 32;
                    D3D12_WRITEBUFFERIMMEDIATE_PARAMETER params[batchSize];
                    UINT64 gpuAddrBase = pDestinationBuffer->GetGPUVirtualAddress() + AlignedDestinationBufferOffset;
                    for (UINT i = 0; i < NumQueries; i += batchSize) {
                        UINT currentBatchCount = (NumQueries - i) < batchSize ? (NumQueries - i) : batchSize;
                        for (UINT j = 0; j < currentBatchCount; ++j) {
                            params[j].Dest = gpuAddrBase + ((UINT64(i) + j) * sizeof(UINT64));
                            params[j].Value = visibleValue;
                        }
                        cl2->WriteBufferImmediate(currentBatchCount, params, nullptr);
                    }
                    cl2->Release();
                    return;
                }
            }
        }
        oResolveQueryDataD3D12(self, pQueryHeap, Type, StartIndex, NumQueries, pDestinationBuffer, AlignedDestinationBufferOffset);
    }

 

    void release() {
        Log("[d3d12hook] Releasing resources and hooks.");
        gShutdown = true;

        if (gInitialized && ImGui::GetCurrentContext()) {
            ImGui_ImplDX12_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            gInitialized = false;
        }

        if (gCommandList) gCommandList->Release();
        if (gHeapRTV) gHeapRTV->Release();
        if (gHeapSRV) gHeapSRV->Release();

        for (UINT i = 0; i < gBufferCount; ++i) {
            if (gFrameContexts[i].renderTarget) gFrameContexts[i].renderTarget->Release();
        }
        if (gOverlayFence) { gOverlayFence->Release(); gOverlayFence = nullptr; }
        if (gFenceEvent) { CloseHandle(gFenceEvent); gFenceEvent = nullptr; }
        if (gCommandQueue) { gCommandQueue->Release(); gCommandQueue = nullptr; }
        if (gDevice) gDevice->Release();
        delete[] gFrameContexts;

        hooks::Remove();
    }

    bool IsInitialized() { return gInitialized; }
}
