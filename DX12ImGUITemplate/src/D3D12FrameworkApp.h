#pragma once
#include <string>
#include <windows.h>
#include "d3d12Util.h"
#include <dxgi1_4.h>
#include <d3d12.h>
#include "d3dx12.h"

using namespace Microsoft::WRL;

struct D3D12_APP_INIT_DESC {
	bool enableDebugLayer;
	std::string appName;
	unsigned int width;
	unsigned int height;
	int backBufferNum;
};

class D3D12FrameworkApp
{
public:
	D3D12FrameworkApp(const D3D12_APP_INIT_DESC& initInfo);
	D3D12FrameworkApp(const D3D12FrameworkApp& rhs) = delete;
	D3D12FrameworkApp& operator=(const D3D12FrameworkApp& rhs) = delete;
	virtual ~D3D12FrameworkApp();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static D3D12FrameworkApp* GetApp();
	
protected:
	static D3D12FrameworkApp* m_appPtr;
	HWND m_mainWndHandle; // main window handle
	HINSTANCE m_appInstHandle;

	ComPtr<IDXGIFactory4> m_dxgiFactory;
	ComPtr<IDXGISwapChain> m_swapChain;
	ComPtr<ID3D12Device> m_d3dDevice;
	ComPtr<ID3D12Fence> m_fence;

	UINT m_RtvDescriptorHandleIncrementSize;
	UINT m_DsvDescriptorHandleIncrementSize;
	UINT m_CbvSrvDescriptorHandleIncrementSize;

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter1* adapter);
};

