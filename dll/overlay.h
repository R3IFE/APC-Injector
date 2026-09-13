
#include <dcomp.h>
#include <wrl.h>
#include <atomic>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "dcomp.lib")

using Microsoft::WRL::ComPtr;


#define FRAME_COUNT 2
static std::atomic<bool> g_running = true;
static HWND g_gameHwnd = nullptr;
static HWND g_overlayHwnd = nullptr;
static bool g_clickThrough = true;

static ComPtr<ID3D12Device> g_device;
static ComPtr<ID3D12CommandQueue> g_queue;
static ComPtr<ID3D12CommandAllocator> g_alloc[FRAME_COUNT];
static ComPtr<ID3D12GraphicsCommandList> g_cmd;
static ComPtr<ID3D12Fence> g_fence;
static HANDLE g_fenceEvent = nullptr;
static UINT64 g_mainFenceValue = 0;
static UINT64 g_frameFenceValues[FRAME_COUNT] = { 0 };

static ComPtr<IDXGISwapChain3> g_swapchain;
static ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
static ComPtr<ID3D12DescriptorHeap> g_srvHeap;
static ComPtr<ID3D12Resource> g_buffers[FRAME_COUNT];
static UINT g_rtvSize = 0;

static ComPtr<IDCompositionDevice> g_dcomp;
static ComPtr<IDCompositionTarget> g_target;
static ComPtr<IDCompositionVisual> g_visual;

static UINT g_width = 0;
static UINT g_height = 0;



void FlushGPU()
{
    if (g_queue && g_fence && g_fenceEvent) {
        g_mainFenceValue++;
        g_queue->Signal(g_fence.Get(), g_mainFenceValue);
        if (g_fence->GetCompletedValue() < g_mainFenceValue) {
            g_fence->SetEventOnCompletion(g_mainFenceValue, g_fenceEvent);
            WaitForSingleObject(g_fenceEvent, INFINITE);
        }
    }
}

RECT GetGameRect()
{
    RECT r{};
    GetWindowRect(g_gameHwnd, &r);
    return r;
}

HWND FindMainGameWindow()
{
    HWND hwnd = nullptr;
    HWND best = nullptr;
    long long bestArea = 0;

    while ((hwnd = FindWindowEx(nullptr, hwnd, nullptr, nullptr)) != nullptr) {
        if (GetParent(hwnd) == nullptr && IsWindowVisible(hwnd)) {
            DWORD windowPid = 0;
            GetWindowThreadProcessId(hwnd, &windowPid);
            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, windowPid);
            if (!hProc) continue;

            char exePath[MAX_PATH] = {};
            DWORD exePathSize = MAX_PATH;
            QueryFullProcessImageNameA(hProc, 0, exePath, &exePathSize);
            CloseHandle(hProc);

            const char* exeName = strrchr(exePath, '\\');
            exeName = exeName ? exeName + 1 : exePath;
            if (_stricmp(exeName, "RainbowSix.exe") != 0) continue;

            RECT rect;
            if (GetWindowRect(hwnd, &rect)) {
                long long w = rect.right - rect.left;
                long long h = rect.bottom - rect.top;
                long long area = w * h;
                if (area > bestArea && w >= 640 && h >= 480) { bestArea = area; best = hwnd; }
            }
        }
    }
    return best;
}


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (!g_clickThrough) {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
            return true;
    }
    switch (msg) {
    case WM_DESTROY: g_running = false; return 0;
    case WM_NCHITTEST: return g_clickThrough ? HTTRANSPARENT : HTCLIENT;
    case WM_SYSCOMMAND: if ((wp & 0xFFF0) == SC_KEYMENU) return 0; break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

HWND CreateOverlayWindow()
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    WNDCLASSEX wc{ sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"OverlayClass";
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName, L"Overlay", WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, wc.hInstance, nullptr);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return hwnd;
}


void InitD3D12()
{
    D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_device));
    D3D12_COMMAND_QUEUE_DESC qd{};
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    g_device->CreateCommandQueue(&qd, IID_PPV_ARGS(&g_queue));
    for (UINT i = 0; i < FRAME_COUNT; i++)
        g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_alloc[i]));
    g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_alloc[0].Get(), nullptr, IID_PPV_ARGS(&g_cmd));
    g_cmd->Close();
    g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence));
    g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void InitDirectComposition(UINT width, UINT height)
{
    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    DXGI_SWAP_CHAIN_DESC1 sd{};
    sd.Width = width; sd.Height = height;
    sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferCount = FRAME_COUNT;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.SampleDesc.Count = 1;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    ComPtr<IDXGISwapChain1> sc1;
    factory->CreateSwapChainForComposition(g_queue.Get(), &sd, nullptr, &sc1);
    sc1.As(&g_swapchain);

    DCompositionCreateDevice(nullptr, IID_PPV_ARGS(&g_dcomp));
    g_dcomp->CreateTargetForHwnd(g_overlayHwnd, TRUE, &g_target);
    g_dcomp->CreateVisual(&g_visual);
    g_visual->SetContent(g_swapchain.Get());
    g_target->SetRoot(g_visual.Get());
    g_dcomp->Commit();
}



static void SetupImGuiStyle()
{
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 6.0f;
    s.FrameRounding = 4.0f;
    s.GrabRounding = 3.0f;
    s.TabRounding = 4.0f;
    s.ScrollbarRounding = 4.0f;
    s.ChildRounding = 4.0f;
    s.PopupRounding = 4.0f;
    s.WindowPadding = ImVec2(10, 10);
    s.FramePadding = ImVec2(6, 4);
    s.ItemSpacing = ImVec2(8, 6);
    s.ItemInnerSpacing = ImVec2(6, 4);
    s.ScrollbarSize = 12.0f;
    s.GrabMinSize = 10.0f;
    s.WindowBorderSize = 1.0f;
    s.FrameBorderSize = 0.0f;
    s.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]            = ImVec4(0.08f, 0.08f, 0.10f, 0.94f);
    c[ImGuiCol_ChildBg]             = ImVec4(0.10f, 0.10f, 0.12f, 0.80f);
    c[ImGuiCol_Border]              = ImVec4(0.30f, 0.30f, 0.35f, 0.50f);
    c[ImGuiCol_FrameBg]             = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
    c[ImGuiCol_FrameBgHovered]      = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
    c[ImGuiCol_FrameBgActive]       = ImVec4(0.28f, 0.28f, 0.36f, 1.00f);
    c[ImGuiCol_TitleBg]             = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    c[ImGuiCol_TitleBgActive]       = ImVec4(0.12f, 0.20f, 0.40f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]    = ImVec4(0.06f, 0.06f, 0.08f, 0.50f);
    c[ImGuiCol_Tab]                 = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    c[ImGuiCol_TabHovered]          = ImVec4(0.30f, 0.45f, 0.75f, 0.80f);
    c[ImGuiCol_TabActive]           = ImVec4(0.20f, 0.35f, 0.65f, 1.00f);
    c[ImGuiCol_Button]              = ImVec4(0.20f, 0.35f, 0.60f, 1.00f);
    c[ImGuiCol_ButtonHovered]       = ImVec4(0.28f, 0.45f, 0.72f, 1.00f);
    c[ImGuiCol_ButtonActive]        = ImVec4(0.15f, 0.28f, 0.50f, 1.00f);
    c[ImGuiCol_CheckMark]           = ImVec4(0.40f, 0.65f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrab]          = ImVec4(0.30f, 0.50f, 0.85f, 1.00f);
    c[ImGuiCol_SliderGrabActive]    = ImVec4(0.40f, 0.60f, 1.00f, 1.00f);
    c[ImGuiCol_Header]              = ImVec4(0.20f, 0.35f, 0.60f, 0.55f);
    c[ImGuiCol_HeaderHovered]       = ImVec4(0.28f, 0.45f, 0.72f, 0.80f);
    c[ImGuiCol_HeaderActive]        = ImVec4(0.20f, 0.35f, 0.65f, 1.00f);
    c[ImGuiCol_Separator]           = ImVec4(0.25f, 0.25f, 0.30f, 0.50f);
    c[ImGuiCol_Text]                = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
    c[ImGuiCol_TextDisabled]        = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
    c[ImGuiCol_ScrollbarBg]         = ImVec4(0.08f, 0.08f, 0.10f, 0.50f);
    c[ImGuiCol_ScrollbarGrab]       = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered]= ImVec4(0.35f, 0.35f, 0.40f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.40f, 0.50f, 1.00f);
}


void InitRTVsAndImGui()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = FRAME_COUNT;
    g_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&g_rtvHeap));
    g_rtvSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
    srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDesc.NumDescriptors = 1;
    srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    g_device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&g_srvHeap));

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < FRAME_COUNT; i++) {
        g_swapchain->GetBuffer(i, IID_PPV_ARGS(&g_buffers[i]));
        g_device->CreateRenderTargetView(g_buffers[i].Get(), nullptr, rtv);
        rtv.ptr += g_rtvSize;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

   
    io.Fonts->AddFontDefault();
    ImFontConfig fontCfg;
    fontCfg.SizePixels = 15.0f;
    fontCfg.OversampleH = 2;
    fontCfg.OversampleV = 1;
    io.FontDefault = io.Fonts->AddFontDefault(&fontCfg);

    SetupImGuiStyle();

    ImGui_ImplWin32_Init(g_overlayHwnd);
    ImGui_ImplDX12_Init(
        g_device.Get(), FRAME_COUNT, DXGI_FORMAT_B8G8R8A8_UNORM,
        g_srvHeap.Get(),
        g_srvHeap->GetCPUDescriptorHandleForHeapStart(),
        g_srvHeap->GetGPUDescriptorHandleForHeapStart());
}


void ResizeOverlayIfNeeded()
{
    if (!IsWindow(g_gameHwnd) || GetWindowLong(g_gameHwnd, GWL_STYLE) == 0) {
        g_gameHwnd = FindMainGameWindow();
        if (!g_gameHwnd) return;
    }
    RECT r = GetGameRect();
    UINT w = r.right - r.left;
    UINT h = r.bottom - r.top;
    if (w <= 0 || h <= 0) return;
    if (w == g_width && h == g_height) return;

    FlushGPU();
    for (UINT i = 0; i < FRAME_COUNT; i++) { g_buffers[i].Reset(); g_frameFenceValues[i] = 0; }
    g_width = w; g_height = h;

    HRESULT hr = g_swapchain->ResizeBuffers(FRAME_COUNT, g_width, g_height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    if (FAILED(hr)) return;

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < FRAME_COUNT; i++) {
        g_swapchain->GetBuffer(i, IID_PPV_ARGS(&g_buffers[i]));
        g_device->CreateRenderTargetView(g_buffers[i].Get(), nullptr, rtv);
        rtv.ptr += g_rtvSize;
    }
    SetWindowPos(g_overlayHwnd, HWND_TOPMOST, r.left, r.top, w, h, SWP_NOACTIVATE);
    if (ImGui::GetCurrentContext()) ImGui::GetIO().DisplaySize = ImVec2((float)w, (float)h);
}

void UpdateInputState()
{
    static bool lastClickThrough = true;
    if (g_clickThrough != lastClickThrough) {
        LONG_PTR exStyle = GetWindowLongPtr(g_overlayHwnd, GWL_EXSTYLE);
        if (g_clickThrough)
            SetWindowLongPtr(g_overlayHwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT | WS_EX_LAYERED);
        else {
            SetWindowLongPtr(g_overlayHwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
            SetForegroundWindow(g_overlayHwnd);
        }
        lastClickThrough = g_clickThrough;
    }
}



static void RenderMenuContent()
{
    {
        std::lock_guard<std::mutex> lk(g_rtMutex);
        if (ImGui::Button("All On"))  { for (int i = 0; i < g_rtEntryCount; i++) g_rtEntries[i].wallhack = true; }
        ImGui::SameLine();
        if (ImGui::Button("All Off")) { for (int i = 0; i < g_rtEntryCount; i++) g_rtEntries[i].wallhack = false; }
        ImGui::SameLine();
        if (ImGui::Button("Active On")) { for (int i = 0; i < g_rtEntryCount; i++) if (g_rtEntries[i].seenThisFrame) g_rtEntries[i].wallhack = true; }
        ImGui::SameLine();
        if (ImGui::Button("Inactive On")) { for (int i = 0; i < g_rtEntryCount; i++) if (!g_rtEntries[i].seenThisFrame) g_rtEntries[i].wallhack = true; }

        int activeCount = 0;
        for (int i = 0; i < g_rtEntryCount; i++) if (g_rtEntries[i].seenThisFrame) activeCount++;
        ImGui::Text("Entries: %d  Active: %d  Inactive: %d", g_rtEntryCount, activeCount, g_rtEntryCount - activeCount);
    }

    ImGui::Checkbox("Collect", &g_rtCollect);
    ImGui::SameLine();
    ImGui::Checkbox("Freeze", &g_rtFreeze);

    ImGui::SliderInt("RT Target", &g_rtTarget, 0, 16);
    ImGui::SameLine();
    ImGui::Checkbox("Filter##RT", &g_rtFilterByTarget);

    ImGui::SliderInt("SH Hash", &g_rtTargetHash, 0, 99);
    ImGui::SameLine();
    ImGui::Checkbox("Filter##SH", &g_rtFilterByHash);

    if (ImGui::Button("Clear")) {
        std::lock_guard<std::mutex> lk(g_rtMutex);
        g_rtEntryCount = 0;
        memset(g_rtEntries, 0, sizeof(g_rtEntries));
        g_dbgAttempts = 0; g_dbgNoEntry = 0; g_dbgNoDev = 0;
        g_dbgNoMods = 0; g_dbgNoFn = 0; g_dbgDescNotFound = 0;
        g_dbgCacheFail = 0; g_dbgCreateFail = 0; g_dbgCreateOK = 0;
        g_dbgApplied = 0;
    }

    ImGui::Checkbox("Enable RT Wallhack", &g_rtWallhackOn);
    ImGui::Checkbox("Hide instead of Wallhack", &g_rtHideMode);
    ImGui::Checkbox("Reverse Depth", &reversedDepth);
    ImGui::Checkbox("Disable Occlusion Culling", &DisableOcclusionCulling);
    ImGui::Text("Skipped: %d/frame", g_asSkippedShown.load());

    ImGui::BeginChild("RTList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    {
        std::lock_guard<std::mutex> lk(g_rtMutex);
        for (int i = 0; i < g_rtEntryCount; i++) {
            char label[320];
            const RTEntry& e = g_rtEntries[i];
            snprintf(label, sizeof(label), "[%d]%s %s PSO:%08X IB:%08X IC:%d MC:%d SH:%d S:%d F:%d RT:%d H:%d T:%d NV:%d VP:%dx%d DSV:%d",
                i, e.wallhack ? "ON " : "OFF", e.isExeInd ? "ExI" : "DII",
                (UINT)((UINT64)e.pso & 0xFFFFFFFF), (UINT)(e.ib & 0xFFFFFFFF),
                e.indexCount, e.maxCmdCount, e.strideHash, e.startSlot, e.format,
                e.numRTVs, e.hash, e.topology, e.numViews, e.vpWidth, e.vpHeight, e.hasDSV ? 1 : 0);

            bool isOn = g_rtEntries[i].wallhack;
            ImVec4 col = e.seenThisFrame ? ImVec4(1.0f, 0.35f, 0.35f, 1.0f) : ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
            if (isOn) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
            else ImGui::PushStyleColor(ImGuiCol_Text, col);

            if (ImGui::Selectable(label, isOn))
                g_rtEntries[i].wallhack = !g_rtEntries[i].wallhack;
            ImGui::PopStyleColor();
        }
    }
    ImGui::EndChild();
}


static bool g_showMenu = false;

void Render()
{
    ResizeOverlayIfNeeded();

    if (GetAsyncKeyState(VK_INSERT) & 1) {
        SaveConfig();
        g_showMenu = !g_showMenu;
        g_clickThrough = !g_showMenu;

        LONG_PTR exStyle = GetWindowLongPtr(g_overlayHwnd, GWL_EXSTYLE);
        if (g_showMenu) {
            SetWindowLongPtr(g_overlayHwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT & ~WS_EX_NOACTIVATE);
            SetForegroundWindow(g_overlayHwnd);
            SetActiveWindow(g_overlayHwnd);
            SetFocus(g_overlayHwnd);
        } else {
            SetWindowLongPtr(g_overlayHwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
            SetForegroundWindow(g_gameHwnd);
        }
    }

    UpdateInputState();

    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }

    RTBeginFrame();

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (g_showMenu) {
        ImGui::SetNextWindowSize(ImVec2(520, 500), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(25, 25), ImGuiCond_FirstUseEver);

        ImGui::Begin("D3D12 Wallhack", &g_showMenu, ImGuiWindowFlags_NoCollapse);

        RenderMenuContent();

        ImGui::End();
    }

    ImGui::Render();

    UINT backBufferIdx = g_swapchain->GetCurrentBackBufferIndex();
    if (g_frameFenceValues[backBufferIdx] != 0 && g_fence->GetCompletedValue() < g_frameFenceValues[backBufferIdx]) {
        g_fence->SetEventOnCompletion(g_frameFenceValues[backBufferIdx], g_fenceEvent);
        WaitForSingleObject(g_fenceEvent, INFINITE);
    }

    g_alloc[backBufferIdx]->Reset();
    g_cmd->Reset(g_alloc[backBufferIdx].Get(), nullptr);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = g_buffers[backBufferIdx].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    g_cmd->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += backBufferIdx * g_rtvSize;

    const float clear[4] = { 0,0,0,0 };
    g_cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    g_cmd->ClearRenderTargetView(rtv, clear, 0, nullptr);
    g_cmd->SetDescriptorHeaps(1, g_srvHeap.GetAddressOf());

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_cmd.Get());

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    g_cmd->ResourceBarrier(1, &barrier);
    g_cmd->Close();

    ID3D12CommandList* lists[] = { g_cmd.Get() };
    g_queue->ExecuteCommandLists(1, lists);
    g_swapchain->Present(1, 0);

    g_mainFenceValue++;
    g_queue->Signal(g_fence.Get(), g_mainFenceValue);
    g_frameFenceValues[backBufferIdx] = g_mainFenceValue;
}



void CleanupOverlay()
{
    if (g_device && g_queue && g_fence && g_fenceEvent) {
        g_mainFenceValue++;
        if (SUCCEEDED(g_queue->Signal(g_fence.Get(), g_mainFenceValue)))
            if (g_fence->GetCompletedValue() < g_mainFenceValue)
                WaitForSingleObject(g_fenceEvent, INFINITE);
    }
    if (ImGui::GetCurrentContext()) { ImGui_ImplDX12_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); }
    if (g_visual) g_visual.Reset();
    if (g_target) g_target.Reset();
    if (g_dcomp) g_dcomp.Reset();
    for (int i = 0; i < FRAME_COUNT; ++i) { g_buffers[i].Reset(); g_alloc[i].Reset(); }
    if (g_swapchain) g_swapchain.Reset();
    if (g_rtvHeap) g_rtvHeap.Reset();
    if (g_srvHeap) g_srvHeap.Reset();
    if (g_cmd) g_cmd.Reset();
    if (g_fence) g_fence.Reset();
    if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
    if (g_queue) g_queue.Reset();
    if (g_device) g_device.Reset();
    if (g_overlayHwnd && IsWindow(g_overlayHwnd)) { DestroyWindow(g_overlayHwnd); g_overlayHwnd = nullptr; }
}

DWORD WINAPI OverlayThread(LPVOID)
{
    while (g_running) {
        g_gameHwnd = FindMainGameWindow();
        if (g_gameHwnd && IsWindow(g_gameHwnd)) break;
        Sleep(200);
    }
    if (!g_running) return 0;

    RECT r = GetGameRect();
    g_overlayHwnd = CreateOverlayWindow();
    MoveWindow(g_overlayHwnd, r.left, r.top, r.right - r.left, r.bottom - r.top, TRUE);

    InitD3D12();
    InitDirectComposition(r.right - r.left, r.bottom - r.top);
    InitRTVsAndImGui();
    LoadConfig();

    while (g_running) {
        if (!IsWindow(g_gameHwnd)) { g_running = false; break; }
        Render();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    CleanupOverlay();
    return 0;
}



using IsInitFn = bool (*)();

static bool WaitForInitialization(IsInitFn fn, int attempts = 50, int sleepMs = 100)
{
    for (int i = 0; i < attempts; ++i) { if (fn()) return true; Sleep(sleepMs); }
    return false;
}

static bool TryInitBackend(globals::Backend backend)
{
    if (backend == globals::Backend::DX12 && (GetModuleHandleA("d3d12.dll") || GetModuleHandleA("dxgi.dll"))) {
        hooks::InitH();
        if (WaitForInitialization(d3d12hook::IsInitialized)) {
            Log("[DllMain] DX12 initialization succeeded.");
            globals::activeBackend = globals::Backend::DX12;
            return true;
        }
        Log("[DllMain] DX12 initialization failed.\n");
        d3d12hook::release();
    }
    return false;
}

static std::atomic<bool> g_BackendInitialized = false;
static std::atomic<bool> g_BackendWatcherRunning = false;

static DWORD WINAPI BackendWatcherThread(LPVOID)
{
    g_BackendWatcherRunning = true;
    while (g_running && !g_BackendInitialized) {
        if (GetModuleHandleA("d3d12.dll") || GetModuleHandleA("dxgi.dll")) {
            if (TryInitBackend(globals::Backend::DX12)) { g_BackendInitialized = true; break; }
        }
        Sleep(100);
    }
    g_BackendWatcherRunning = false;
    return 0;
}

static DWORD WINAPI onAttach(LPVOID)
{
    TlsInit();
    HANDLE hThread = CreateThread(nullptr, 0, BackendWatcherThread, nullptr, 0, nullptr);
    if (hThread) CloseHandle(hThread);
    CreateThread(nullptr, 0, OverlayThread, nullptr, 0, nullptr);
    return 0;
}

void ReleaseActiveBackend()
{
    if (globals::activeBackend == globals::Backend::DX12) d3d12hook::release();
    globals::activeBackend = globals::Backend::None;
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        globals::mainModule = hModule;
        { HANDLE thread = CreateThread(nullptr, 0, onAttach, nullptr, 0, nullptr); if (thread) CloseHandle(thread); }
        break;
    case DLL_PROCESS_DETACH:
        g_running = false;
        g_BackendInitialized = true;
        Sleep(100);
        CleanupColorBuffer();
        ReleaseActiveBackend();
        if (ImGui::GetCurrentContext()) { ImGui_ImplDX12_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); }
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport)
LRESULT CALLBACK NextHook(int code, WPARAM wParam, LPARAM lParam)
{
    return CallNextHookEx(NULL, code, wParam, lParam);
}
