#pragma once
#include <d3d12.h>
#include <iostream>
#include <string>
#include <wrl.h>
#include <tchar.h>
#include <dxgi1_4.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

using namespace Microsoft::WRL;

struct D3D12_APP_INIT_DESC {
	bool enableDebugLayer;
	std::string appName;
	unsigned int width;
	unsigned int height;
	int backBufferNum;
};

struct FrameContext
{
	ID3D12CommandAllocator* CommandAllocator;
	UINT64                  FenceValue;
};

class D3D12App
{
public:
	static ComPtr<ID3D12Device> m_d3dDevice;

	D3D12App(const D3D12_APP_INIT_DESC& initInfo);
	~D3D12App();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;
	static void ResizeSwapChain(HWND hWnd, int width, int height);
	static void WaitForLastSubmittedFrame();
	static void CreateSCBRenderTargetView();	// SCB: Swapchain buffers.

private:
	static HWND m_winHandle;
	ComPtr<IDXGIFactory4> m_dxgiFactory;
	static ComPtr<ID3D12Fence> m_fence;
	static ComPtr<ID3D12CommandQueue> m_cmdQueue;
	ComPtr<ID3D12GraphicsCommandList> m_cmdList;
	static ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap;
	static ID3D12Resource* m_pSwapChainBuffer[2];
	ComPtr<ID3D12Resource> m_pDepthStencilBuffer;

	UINT m_rtvDescriptorSize;
	UINT m_dsvDescritporSize;
	UINT m_cbvsrvDescriptorSize;

	UINT m_clientWidth;
	UINT m_clientHeight;
	UINT m_curBackBufferId;
	static int m_backBufferNum;

	// DearImGUI
	static UINT m_frameIndex;
	static HANDLE m_fenceEvent;
	static HANDLE m_hSwapChainWaitableObject;
	UINT64 m_fenceLastSignaledValue;
	static D3D12_CPU_DESCRIPTOR_HANDLE m_mainRenderTargetDescriptor[2];
	static FrameContext m_frameContext[3];

	FrameContext* WaitForNextFrameResources();
	
};

