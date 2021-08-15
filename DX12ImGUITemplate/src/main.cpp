#include "D3D12App.h"
#include "d3d12Util.h"
int main(int, char**) {
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    
    try
    {
        std::cout << "Hello World!" << std::endl;
        D3D12_APP_INIT_DESC initInfo = {};
        {
            initInfo.appName = "D3D12 GUI Template";
            initInfo.height = 960;
            initInfo.width = 1280;
            initInfo.enableDebugLayer = true;
            initInfo.backBufferNum = 2;
        }
        D3D12App mApp(initInfo);
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
    
}


