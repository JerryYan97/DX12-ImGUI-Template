#pragma once
#include <d3d12.h>
#include <string>
#include <comdef.h>


inline std::wstring AnsiToWString(const std::string& str) {
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& funcName, const std::wstring& filename, int lineNumber) :
		ErrorCode(hr), FuncName(funcName), Filename(filename), LineNumber(lineNumber)
	{}

	std::wstring ToString() const {
		_com_error err(this->ErrorCode);
		std::wstring msg = err.ErrorMessage();
		return this->FuncName + L"failed in" + this->Filename + L";line " + std::to_wstring(this->LineNumber) + L"; error: " + msg;
	}

	HRESULT ErrorCode = S_OK;
	std::wstring FuncName;
	std::wstring Filename;
	int LineNumber = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)												\
{																		\
	HRESULT hr__ = (x);													\
	std::wstring wfn = AnsiToWString(__FILE__);							\
	if(FAILED(hr__)){ throw DxException(hr__, L#x, wfn, __LINE__); }	\
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif