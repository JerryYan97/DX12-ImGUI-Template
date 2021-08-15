#include "D3D12App.h"
#include "d3d12Util.h"
#include "d3dx12.h"
#include <dxgidebug.h>

// Static definition
ComPtr<ID3D12Device> D3D12App::m_d3dDevice;
ComPtr<ID3D12Fence> D3D12App::m_fence;
ComPtr<ID3D12CommandQueue> D3D12App::m_cmdQueue;
ComPtr<IDXGISwapChain3> D3D12App::m_swapChain;
ID3D12Resource* D3D12App::m_pSwapChainBuffer[2];
int D3D12App::m_backBufferNum;
UINT D3D12App::m_frameIndex;
HANDLE D3D12App::m_fenceEvent;
HANDLE D3D12App::m_hSwapChainWaitableObject;
D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::m_mainRenderTargetDescriptor[2];
FrameContext D3D12App::m_frameContext[3];

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


D3D12App::~D3D12App() {

}

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        ImGui_ImplDX12_CreateDeviceObjects();
        if (D3D12App::m_d3dDevice.Get() != NULL && wParam != SIZE_MINIMIZED)
        {
            D3D12App::WaitForLastSubmittedFrame();
            ImGui_ImplDX12_InvalidateDeviceObjects();
            D3D12App::ResizeSwapChain(hWnd, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
            D3D12App::CreateSCBRenderTargetView();
            ImGui_ImplDX12_CreateDeviceObjects();
        }
        
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}


void D3D12App::WaitForLastSubmittedFrame()
{
    FrameContext* frameCtx = &m_frameContext[m_frameIndex % 3];

    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue == 0)
        return; // No fence was signaled

    frameCtx->FenceValue = 0;
    if (m_fence->GetCompletedValue() >= fenceValue)
        return;

    m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
    WaitForSingleObject(m_fenceEvent, INFINITE);
}

FrameContext* D3D12App::WaitForNextFrameResources() {
    UINT nextFrameIndex = m_frameIndex + 1;
    m_frameIndex = nextFrameIndex;

    HANDLE waitableObjects[] = { m_hSwapChainWaitableObject, NULL };
    DWORD numWaitableObjects = 1;

    FrameContext* frameCtx = &m_frameContext[nextFrameIndex % 3];
    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue != 0) // means no fence was signaled
    {
        frameCtx->FenceValue = 0;
        m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
        waitableObjects[1] = m_fenceEvent;
        numWaitableObjects = 2;
    }

    WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

    return frameCtx;
}

void D3D12App::CreateSCBRenderTargetView() {
    for (int i = 0; i < m_backBufferNum; ++i) {
        ID3D12Resource* pBackBuffer = nullptr;
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        m_d3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, m_mainRenderTargetDescriptor[i]);
    }
}

void D3D12App::ResizeSwapChain(HWND hWnd, int width, int height) {
    WaitForLastSubmittedFrame(); 
    for (size_t i = 0; i < m_backBufferNum; i++)
    {
        m_pSwapChainBuffer[i]->Release();
        m_pSwapChainBuffer[i] = nullptr;
    }

    DXGI_SWAP_CHAIN_DESC1 sd;
    m_swapChain->GetDesc1(&sd);
    sd.Width = width;
    sd.Height = height;

    IDXGIFactory4* dxgiFactory = nullptr;
    m_swapChain->GetParent(IID_PPV_ARGS(&dxgiFactory));
    CloseHandle(m_hSwapChainWaitableObject);
    m_swapChain->Release();
    m_swapChain.Reset();

    IDXGISwapChain1* swapChain1 = nullptr;
    dxgiFactory->CreateSwapChainForHwnd(m_cmdQueue.Get(), hWnd, &sd, nullptr, nullptr, &swapChain1);
    swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain));
    swapChain1->Release();
    dxgiFactory->Release();

    m_swapChain->SetMaximumFrameLatency(m_backBufferNum);

    m_hSwapChainWaitableObject = m_swapChain->GetFrameLatencyWaitableObject();
    assert(m_hSwapChainWaitableObject != nullptr);

    // Get new swapchain buffers' pointers
    for (size_t i = 0; i < m_backBufferNum; i++)
    {
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_pSwapChainBuffer[i]));
    }
}

D3D12App::D3D12App(const D3D12_APP_INIT_DESC& initInfo) {
    // ImGui init
    m_frameIndex = 0;
    m_fenceEvent = nullptr;
    m_clientHeight = initInfo.height;
    m_clientWidth = initInfo.width;
    m_backBufferNum = initInfo.backBufferNum;

	// Create DearImgui Windows
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
	::RegisterClassEx(&wc);
	HWND m_winHandle = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX12 Example"), WS_OVERLAPPEDWINDOW, 100, 100, m_clientWidth, m_clientHeight, NULL, NULL, wc.hInstance, NULL);

    UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();

        // Enable additional debug layers.
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif // _DEBUG


    // Initialize Direct3D hardware device
    
    ThrowIfFailed(D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_d3dDevice)
    ));
    
    // D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_d3dDevice));

    // Init D3d Factory
    // ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&this->m_dxgiFactory)));
    CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&this->m_dxgiFactory));

    // Init fence
    // ThrowIfFailed(m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));

    // Query descriptors size
    m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescritporSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_cbvsrvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Init D3d cmd queue, allocator and list
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    {
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    }
    
    ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_cmdQueue)));
    for (UINT i = 0; i < 3; i++) {
        ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_frameContext[i].CommandAllocator)));
    }
    ThrowIfFailed(m_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&m_cmdList)));

    // Create Swap Chain
    DXGI_SWAP_CHAIN_DESC1 sd = {};
    {
        sd.BufferCount = 2;
        sd.Width = initInfo.width;
        sd.Height = initInfo.height;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Count = 1;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

    IDXGISwapChain1* swapChain1 = NULL;
    ThrowIfFailed(m_dxgiFactory->CreateSwapChainForHwnd(m_cmdQueue.Get(), m_winHandle, &sd, nullptr, nullptr, &swapChain1));
    ThrowIfFailed(swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain)));
    swapChain1->Release();
    m_swapChain->SetMaximumFrameLatency(2);
    m_hSwapChainWaitableObject = m_swapChain->GetFrameLatencyWaitableObject();
    // Create Descriptor Heaps and Descriptors
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    {
        rtvHeapDesc.NumDescriptors = 2;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvHeapDesc.NodeMask = 0;
    }
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    {
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NodeMask = 0;
    }
    D3D12_DESCRIPTOR_HEAP_DESC srvCbvUavDesc = {};
    {
        srvCbvUavDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvCbvUavDesc.NumDescriptors = 1;
        srvCbvUavDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }
    ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
    ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
    ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&srvCbvUavDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));

    // Create Render target view for the swapchain's two buffers and get two pointers to the buffers' resources
    // CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    m_mainRenderTargetDescriptor[0] = rtvHandle;
    rtvHandle.ptr += m_rtvDescriptorSize;
    m_mainRenderTargetDescriptor[1] = rtvHandle;
    ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_pSwapChainBuffer[0])));
    ThrowIfFailed(m_swapChain->GetBuffer(1, IID_PPV_ARGS(&m_pSwapChainBuffer[1])));
    CreateSCBRenderTargetView();

    // Create the Depth/Stencil Buffer and View
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    {
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Alignment = 0;
        depthStencilDesc.Width = initInfo.width;
        depthStencilDesc.Height = initInfo.height;
        depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }

    D3D12_CLEAR_VALUE optClear = {};
    {
        optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        optClear.DepthStencil.Depth = 1.f;
        optClear.DepthStencil.Stencil = 0;
    }

    CD3DX12_HEAP_PROPERTIES chp(D3D12_HEAP_TYPE_DEFAULT);
    
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &chp,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(m_pDepthStencilBuffer.GetAddressOf())));
    
    m_d3dDevice->CreateDepthStencilView(m_pDepthStencilBuffer.Get(), nullptr, GetDepthStencilView());

    CD3DX12_RESOURCE_BARRIER tb = CD3DX12_RESOURCE_BARRIER::Transition(m_pDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    m_cmdList->ResourceBarrier(1, &tb);

    // Create render target view for swapchain's back buffers:


    // Show the window
    ::ShowWindow(m_winHandle, SW_SHOWDEFAULT);
    ::UpdateWindow(m_winHandle);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(m_winHandle);
    ImGui_ImplDX12_Init(m_d3dDevice.Get(), 3, DXGI_FORMAT_R8G8B8A8_UNORM, m_cbvSrvHeap.Get(), m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
    
    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ThrowIfFailed(m_cmdList->Close());

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();

        FrameContext* frameCtx = WaitForNextFrameResources();
        UINT backBufferIdx = m_swapChain->GetCurrentBackBufferIndex();
        frameCtx->CommandAllocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        {
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = m_pSwapChainBuffer[backBufferIdx];
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        }
        
        m_cmdList->Reset(frameCtx->CommandAllocator, NULL);
        m_cmdList->ResourceBarrier(1, &barrier);

        // Render Dear ImGui graphics
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        m_cmdList->ClearRenderTargetView(m_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, NULL);
        m_cmdList->OMSetRenderTargets(1, &m_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
        m_cmdList->SetDescriptorHeaps(1, m_cbvSrvHeap.GetAddressOf());
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_cmdList.Get());
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        m_cmdList->ResourceBarrier(1, &barrier);
        m_cmdList->Close();
        ID3D12GraphicsCommandList* pTmpCmdList = m_cmdList.Get();
        m_cmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&pTmpCmdList);

        m_swapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync

        UINT64 fenceValue = m_fenceLastSignaledValue + 1;
        m_cmdQueue->Signal(m_fence.Get(), fenceValue);
        m_fenceLastSignaledValue = fenceValue;
        frameCtx->FenceValue = fenceValue;
    }

}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::GetCurrentBackBufferView() const{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        m_curBackBufferId,
        m_rtvDescriptorSize
    );
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12App::GetDepthStencilView() const {
    return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}