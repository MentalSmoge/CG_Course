// MySuper3DApp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#pragma once

#include <windows.h>
#include <WinUser.h>
#include <wrl.h>
#include <iostream>
#include <d3d.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <chrono>
#include "DisplayWin32.h"
#include "DirectXDevice.h"
#include "ShaderProgram.h"
#include "TriangleComponent.h"
#include <cmath>
#include "MySuper3DApp.h"
#include <vector>
#include "Game.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")



int main()
{
	Game game;

	MSG msg = {};
	bool isExitRequested = false;
	while (!isExitRequested) {
		// Handle the windows messages.
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if (msg.message == WM_QUIT) {
			isExitRequested = true;
		}
		game.Run();
		
	}

    std::cout << "Hello World!\n";
}

