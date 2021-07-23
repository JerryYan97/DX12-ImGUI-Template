#pragma once
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <d3d12.h>
#include <iostream>
#include <string>

struct D3D12_APP_INIT_DESC {
	std::string appName;
	unsigned int width;
	unsigned int height;
};

class D3D12App
{
public:
	D3D12App(const D3D12_APP_INIT_DESC& initInfo);
};

