#pragma once
#include <d3d12.h>
#include <string>

inline std::wstring AnsiToWString(const std::string& str) {
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)		\
{								\
	HRESULT hr__ = (x);			\
	std::wstring wfn = ANSITo
}
#endif
