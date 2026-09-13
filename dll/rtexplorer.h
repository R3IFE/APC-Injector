//rtexplorer.h - PSO registry + RT Explorer / Auto-Skip Scene Draw
#pragma once

#include <cstdint>
#include <mutex>
#include <atomic>
#include <d3d12.h>

// ============================================================
// RT EXPLORER - PSO REGISTRY
// ============================================================

struct RTEntry {
    ID3D12PipelineState* pso = nullptr;
    uint64_t             ib = 0;
    uint32_t             indexCount = 0;
    uint32_t             maxCmdCount = 0;
    uint32_t             strideHash = 0;
    uint32_t             startSlot = 0;
    uint32_t             format = 0;
    uint32_t             numRTVs = 0;
    uint32_t             hash = 0;
    uint32_t             topology = 0;
    uint32_t             numViews = 0;
    uint32_t             vpWidth = 0;
    uint32_t             vpHeight = 0;
    bool                 hasDSV = false;
    bool                 isExeInd = false;
    bool                 wallhack = false;
    bool                 seenThisFrame = false;
    uint64_t             lastSeenFrame = 0;
};

static const int RT_MAX_ENTRIES = 512;

static RTEntry      g_rtEntries[RT_MAX_ENTRIES] = {};
static int          g_rtEntryCount = 0;
static std::mutex   g_rtMutex;
static uint64_t     g_rtFrameCounter = 0;

static bool g_rtCollect        = true;
static bool g_rtFreeze         = false;
static bool g_rtWallhackOn     = true;
static bool g_rtHideMode       = false;
static int  g_eiMaxCmdCount    = 500;
static int  g_rtTarget         = 4;
static bool g_rtFilterByTarget = true;
static bool g_rtFilterByHash   = true;
static int  g_rtTargetHash     = 97;

// ModPSO debug counters
static std::atomic<uint32_t> g_dbgAttempts{0};
static std::atomic<uint32_t> g_dbgNoEntry{0};
static std::atomic<uint32_t> g_dbgNoDev{0};
static std::atomic<uint32_t> g_dbgNoMods{0};
static std::atomic<uint32_t> g_dbgNoFn{0};
static std::atomic<uint32_t> g_dbgDescNotFound{0};
static std::atomic<uint32_t> g_dbgCacheFail{0};
static std::atomic<uint32_t> g_dbgCreateFail{0};
static std::atomic<uint32_t> g_dbgCreateOK{0};
static std::atomic<uint32_t> g_dbgApplied{0};

// Per-frame counters
static std::atomic<uint32_t> g_asSkippedThisFrame{0};
static std::atomic<uint32_t> g_asSkippedShown{0};

static uint32_t RTComputeHash(uint64_t ib, uint32_t strideHash, uint32_t startSlot, uint32_t format)
{
    uint32_t h = 2166136261u;
    auto mix = [&](uint32_t v) { h ^= v; h *= 16777619u; };
    mix((uint32_t)(ib & 0xFFFFFFFF));
    mix((uint32_t)(ib >> 32));
    mix(strideHash);
    mix(startSlot);
    mix(format);
    return h & 0xFFFF;
}

static RTEntry* RTRegisterDraw(
    ID3D12PipelineState* pso,
    uint64_t ib,
    uint32_t indexCount,
    uint32_t strideHash,
    uint32_t startSlot,
    uint32_t format,
    uint32_t numRTVs,
    uint32_t maxCmdCount = 0,
    uint32_t topology = 0,
    uint32_t numViews = 0,
    uint32_t vpWidth = 0,
    uint32_t vpHeight = 0,
    bool hasDSV = false,
    bool isExeInd = false)
{
    std::lock_guard<std::mutex> lk(g_rtMutex);

    const bool matches =
        (!g_rtFilterByTarget || numRTVs == (uint32_t)g_rtTarget) &&
        (!g_rtFilterByHash   || strideHash == (uint32_t)g_rtTargetHash);

    for (int i = 0; i < g_rtEntryCount; ++i) {
        if (g_rtEntries[i].pso == pso) {
            if (!matches) {
                g_rtEntries[i] = g_rtEntries[--g_rtEntryCount];
                return nullptr;
            }
            RTEntry& e = g_rtEntries[i];
            e.ib = ib;
            e.indexCount = indexCount;
            e.maxCmdCount = maxCmdCount;
            e.strideHash = strideHash;
            e.startSlot = startSlot;
            e.format = format;
            e.numRTVs = numRTVs;
            e.hash = RTComputeHash(ib, strideHash, startSlot, format);
            e.topology = topology;
            e.numViews = numViews;
            e.vpWidth = vpWidth;
            e.vpHeight = vpHeight;
            e.hasDSV = hasDSV;
            e.isExeInd = isExeInd;
            e.seenThisFrame = true;
            e.lastSeenFrame = g_rtFrameCounter;
            return &e;
        }
    }

    if (!matches)
        return nullptr;

    if (g_rtEntryCount < RT_MAX_ENTRIES) {
        RTEntry& e = g_rtEntries[g_rtEntryCount++];
        e.pso = pso;
        e.ib = ib;
        e.indexCount = indexCount;
        e.strideHash = strideHash;
        e.startSlot = startSlot;
        e.format = format;
        e.numRTVs = numRTVs;
        e.hash = RTComputeHash(ib, strideHash, startSlot, format);
        e.topology = topology;
        e.numViews = numViews;
        e.vpWidth = vpWidth;
        e.vpHeight = vpHeight;
        e.hasDSV = hasDSV;
        e.isExeInd = isExeInd;
        e.wallhack = false;
        e.seenThisFrame = true;
        e.lastSeenFrame = g_rtFrameCounter;
        return &e;
    }
    return nullptr;
}

static void RTBeginFrame()
{
    g_asSkippedShown.store(g_asSkippedThisFrame.exchange(0, std::memory_order_relaxed), std::memory_order_relaxed);

    if (g_rtFreeze) return;
    std::lock_guard<std::mutex> lk(g_rtMutex);

    g_rtFrameCounter++;
    const uint64_t window = 10;

    for (int i = 0; i < g_rtEntryCount; ) {
        RTEntry& e = g_rtEntries[i];
        const bool matches =
            (!g_rtFilterByTarget || e.numRTVs == (uint32_t)g_rtTarget) &&
            (!g_rtFilterByHash   || e.strideHash == (uint32_t)g_rtTargetHash);
        if (!matches) {
            g_rtEntries[i] = g_rtEntries[--g_rtEntryCount];
            continue;
        }
        e.seenThisFrame = (g_rtFrameCounter - e.lastSeenFrame) < window;
        ++i;
    }
}

static bool RTIsWallhackEnabled(ID3D12PipelineState* pso)
{
    std::lock_guard<std::mutex> lk(g_rtMutex);
    for (int i = 0; i < g_rtEntryCount; ++i) {
        if (g_rtEntries[i].pso == pso)
            return g_rtEntries[i].wallhack;
    }
    return false;
}

// ============================================================
// AUTO-SKIP SCENE DRAW
// ============================================================

static bool g_asEnabled      = true;
static int  g_asMinCmd       = -1;
static int  g_asMaxCmd       = 50000;
static bool g_asRequireCount = true;
static int  g_asStride       = -1;
static int  g_asNumRTVs      = 0;
static int  g_asDrawType     = 0;
static int  g_asMinVPWidth   = -1;
static int  g_asMaxVPWidth   = 4096;
static int  g_asSkipInterval = 0;

static int RTDrawTypeToTopology(int drawType)
{
    switch (drawType) {
    case 1: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case 2: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case 3: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case 4: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    default: return -1;
    }
}

static bool RTFilterMatch(
    int minCmd, int maxCmd, bool requireCount,
    int wantStride, int wantNumRTVs, int drawType,
    int minVPWidth, int maxVPWidth,
    uint32_t cmdCount, bool hasCountBuffer,
    uint32_t strideHash, uint32_t numRTVs,
    uint32_t topology, float vpWidth)
{
    if (requireCount && !hasCountBuffer) return false;
    if (minCmd > 0 && (int)cmdCount < minCmd) return false;
    if (maxCmd > 0 && (int)cmdCount > maxCmd) return false;
    if (wantStride >= 0 && (int)strideHash != wantStride) return false;
    if (wantNumRTVs >= 0 && (int)numRTVs != wantNumRTVs) return false;
    int wantTopo = RTDrawTypeToTopology(drawType);
    if (wantTopo >= 0 && (int)topology != wantTopo) return false;
    if (minVPWidth >= 0 && vpWidth < (float)minVPWidth) return false;
    if (maxVPWidth >= 0 && vpWidth > (float)maxVPWidth) return false;
    return true;
}

static bool RTHasDiscriminator(
    int minCmd, int maxCmd, bool requireCount,
    int wantStride, int wantNumRTVs, int drawType,
    int minVPWidth, int maxVPWidth)
{
    if (requireCount) return true;
    if (minCmd > 0) return true;
    if (wantStride >= 0) return true;
    if (wantNumRTVs > 0) return true;
    if (drawType != 0) return true;
    if (minVPWidth >= 0) return true;
    if (maxVPWidth >= 0 && maxVPWidth < 4096) return true;
    return false;
}

static std::atomic<int> g_asSkipAccum{0};
static bool RTAutoSkipShouldSkip(int interval)
{
    if (interval <= 0) return true;
    if (g_asSkipAccum.fetch_add(1, std::memory_order_relaxed) + 1 < interval)
        return true;
    g_asSkipAccum.store(0, std::memory_order_relaxed);
    return false;
}
