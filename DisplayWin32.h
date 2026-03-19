#pragma once

#include <windows.h>
#include <WinUser.h>
#include <wrl.h>

class DisplayWin32
{
public:
	int ClientHeight = 800;
	int ClientWidth = 800;
	WNDCLASSEX wc{};
	HINSTANCE hInstance = GetModuleHandle(nullptr);
	HWND hWnd{};
	//Module
	DisplayWin32(LPCWSTR applicationName);
};