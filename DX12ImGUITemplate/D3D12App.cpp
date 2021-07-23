#include "D3D12App.h"
#include <wrl.h>


D3D12App::D3D12App(const D3D12_APP_INIT_DESC& initInfo) {

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed
#endif

	// Create DearImgui Windows
	// WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
	// ::
}
