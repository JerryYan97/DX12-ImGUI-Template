#include "D3D12FrameworkApp.h"

#include <iostream>
#include <tchar.h>
#include <assert.h>
#include <wrl.h>

D3D12FrameworkApp* D3D12FrameworkApp::m_appPtr = nullptr;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return D3D12FrameworkApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3D12FrameworkApp::D3D12FrameworkApp(const D3D12_APP_INIT_DESC& initInfo)
    : m_mainWndHandle(nullptr)
{
    /*
        Init Main Window
    */
    assert(m_appPtr == nullptr);
    m_appPtr = this;
    WNDCLASSEX wc;
    {
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = MainWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hIcon = nullptr;
        wc.hCursor = nullptr;
        wc.hbrBackground = nullptr;
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = _T("Dear ImGui DirectX12 Example");
        wc.hIconSm = nullptr;
    }
    
    if (!::RegisterClassEx(&wc))
    {
        MessageBox(0, L"RegisterClass Failed.", 0, 0);
    }

    RECT R = {0, 0, initInfo.width, initInfo.height};
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    HWND m_mainWndHandle = ::CreateWindow(wc.lpszClassName,
                                          _T("Dear ImGui DirectX12 Example"),
                                          WS_OVERLAPPEDWINDOW,
                                          CW_USEDEFAULT,
                                          CW_USEDEFAULT,
                                          width,
                                          height,
                                          nullptr,
                                          nullptr,
                                          wc.hInstance,
                                          nullptr);
    if (!m_mainWndHandle)
    {
        MessageBox(0, L"CreateWindow Failed.", 0, 0);
    }
    else
    {
        ShowWindow(m_mainWndHandle, SW_SHOW);
        UpdateWindow(m_mainWndHandle);
    }

    /*
        Init D3D
    */
#if defined(DEBUG) || defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif

    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)));

    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_d3dDevice)));

    ThrowIfFailed(m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

    m_RtvDescriptorHandleIncrementSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_DsvDescriptorHandleIncrementSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_CbvSrvDescriptorHandleIncrementSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    // TODO: MSAAx4 Support and Open.

#ifdef _DEBUG
    LogAdapters();
#endif

}


D3D12FrameworkApp::~D3D12FrameworkApp() 
{
   
}

D3D12FrameworkApp* D3D12FrameworkApp::GetApp()
{
    return m_appPtr;
}

LRESULT D3D12FrameworkApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_ACTIVATE:
        std::cout << "WM_ACTIVATE event." << std::endl;
        return 0;
    case WM_SIZE:
        std::cout << "WM_SIZE event." << std::endl;
        return 0;
    case WM_ENTERSIZEMOVE:
        std::cout << "WM_ENTERSIZEMOVE event." << std::endl;
        return 0;
    case WM_EXITSIZEMOVE:
        std::cout << "WM_EXITSIZEMOVE event." << std::endl;
        return 0;
    case WM_DESTROY:
        std::cout << "WM_DESTROY event." << std::endl;
        return 0;
    case WM_MENUCHAR:
        std::cout << "WM_MENUCHAR event." << std::endl;
        return 0;
        // Catch this message so to prevent the window from becoming too small.
    case WM_GETMINMAXINFO:
        ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
        ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
        return 0;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        return 0;
    case WM_MOUSEMOVE:
        return 0;
    case WM_KEYUP:
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


void D3D12FrameworkApp::LogAdapters()
{
    IDXGIAdapter1* adapter = nullptr;
    std::vector<IDXGIAdapter1*> adapterList;
    for (UINT i = 0; m_dxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());
        std::wcout << text.c_str() << std::endl;

        adapterList.push_back(adapter);
    }

    for (size_t i = 0; i < adapterList.size(); i++)
    {
        IDXGIAdapter1* tmpAdapter = adapterList[i];
        LogAdapterOutputs(tmpAdapter);
        ReleaseCom(tmpAdapter);
    }
}

/*
    An adapter output is a monitor connected to this adapter(Graphics card).
*/
void D3D12FrameworkApp::LogAdapterOutputs(IDXGIAdapter1* adapter)
{
    IDXGIOutput1* output = nullptr;
    for (UINT i = 0; adapter->EnumOutputs(i, (IDXGIOutput**)&output) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());
        std::wcout << text.c_str() << std::endl;
        ReleaseCom(output);
    }
}