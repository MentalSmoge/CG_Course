#pragma once
#include <d3d.h>
#include <d3d11.h>

class DirectXDevice
{
public:
    DirectXDevice(HWND hWnd, int width, int height);
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
};