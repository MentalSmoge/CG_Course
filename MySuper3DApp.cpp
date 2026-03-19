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

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")



int main()
{
	//1 Create a Window
	LPCWSTR applicationName = L"My3DApp";
	DisplayWin32 displayWin32(applicationName);

	//2 Create Device with the SwapChain
	DirectXDevice directXDevice(displayWin32.hWnd, displayWin32.ClientWidth, displayWin32.ClientHeight);

	//4 Compile the Shaders
	D3D11_INPUT_ELEMENT_DESC inputElements[] = {
	D3D11_INPUT_ELEMENT_DESC {
		"POSITION",
		0,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		0,
		0,
		D3D11_INPUT_PER_VERTEX_DATA,
		0},
	D3D11_INPUT_ELEMENT_DESC {
		"COLOR",
		0,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		0,
		D3D11_APPEND_ALIGNED_ELEMENT,
		D3D11_INPUT_PER_VERTEX_DATA,
		0}
	};
	D3D_SHADER_MACRO Shader_Macros[] = { "TEST", "1", "TCOLOR", "float4(0.0f, 1.0f, 0.0f, 1.0f)", nullptr, nullptr };
	ShaderProgram shaderProgram(
		directXDevice.device,
		L"./Shaders/MyVeryFirstShader.hlsl",
		inputElements,
		2,
		Shader_Macros
	);

	//6 Create set of points
	DirectX::XMFLOAT4 points[8] = {
		DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f),	DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
		DirectX::XMFLOAT4(-0.5f, -0.5f, 0.5f, 1.0f),	DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
		DirectX::XMFLOAT4(0.5f, -0.5f, 0.5f, 1.0f),	DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),
		DirectX::XMFLOAT4(-0.5f, 0.5f, 0.5f, 1.0f),	DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
	};

	//7 Create vertex and index buffers
	/*D3D11_BUFFER_DESC vertexBufDesc = {};
	vertexBufDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufDesc.CPUAccessFlags = 0;
	vertexBufDesc.MiscFlags = 0;
	vertexBufDesc.StructureByteStride = 0;
	vertexBufDesc.ByteWidth = sizeof(DirectX::XMFLOAT4) * std::size(points);

	D3D11_SUBRESOURCE_DATA vertexData = {};
	vertexData.pSysMem = points;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	ID3D11Buffer* vb;
	directXDevice.device->CreateBuffer(&vertexBufDesc, &vertexData, &vb);

	int indeces[] = { 0,1,2, 1,0,3 };
	D3D11_BUFFER_DESC indexBufDesc = {};
	indexBufDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufDesc.CPUAccessFlags = 0;
	indexBufDesc.MiscFlags = 0;
	indexBufDesc.StructureByteStride = 0;
	indexBufDesc.ByteWidth = sizeof(int) * std::size(indeces);

	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = indeces;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	ID3D11Buffer* ib;
	directXDevice.device->CreateBuffer(&indexBufDesc, &indexData, &ib);

	UINT strides[] = { 32 };
	UINT offsets[] = { 0 };*/


	//10.1 Setup Rasterizer Stage 
	CD3D11_RASTERIZER_DESC rastDesc = {};
	rastDesc.CullMode = D3D11_CULL_NONE;
	rastDesc.FillMode = D3D11_FILL_SOLID;

	ID3D11RasterizerState* rastState;
	auto res = directXDevice.device->CreateRasterizerState(&rastDesc, &rastState);

	directXDevice.context->RSSetState(rastState);




	std::chrono::time_point<std::chrono::steady_clock> PrevTime = std::chrono::steady_clock::now();
	float totalTime = 0;
	unsigned int frameCount = 0;

	TriangleComponent::Vertex first_triangle[3] = {
		{DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f),	DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
		{DirectX::XMFLOAT4(-0.5f, -0.5f, 0.5f, 1.0f),	DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
		{DirectX::XMFLOAT4(0.5f, -0.5f, 0.5f, 1.0f),	DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)}
	};

	TriangleComponent triangle(directXDevice.device, first_triangle);

	std::vector<TriangleComponent> triangles = { triangle };

	MSG msg = {};
	bool isExitRequested = false;
	float totalTimeElapsed = 0;
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

		#pragma region SetupStateAndViewport
		directXDevice.context->ClearState();

		directXDevice.context->RSSetState(rastState);

		//10.2 Setup ViewPort
		D3D11_VIEWPORT viewport = {};
		viewport.Width = static_cast<float>(displayWin32.ClientWidth);
		viewport.Height = static_cast<float>(displayWin32.ClientHeight);
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1.0f;

		directXDevice.context->RSSetViewports(1, &viewport);

		//8 Setup the IA stage
		directXDevice.context->IASetInputLayout(shaderProgram.inputLayout);
		directXDevice.context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//9 Set vertex and pixel shaders
		directXDevice.context->VSSetShader(shaderProgram.vertexShader, nullptr, 0);
		directXDevice.context->PSSetShader(shaderProgram.pixelShader, nullptr, 0);
		#pragma endregion

		


		auto	curTime = std::chrono::steady_clock::now();
		float	deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(curTime - PrevTime).count() / 1000000.0f;
		PrevTime = curTime;

		totalTimeElapsed += deltaTime;
		totalTime += deltaTime;
		frameCount++;

		if (totalTime > 1.0f) {
			float fps = frameCount / totalTime;

			totalTime -= 1.0f;

			WCHAR text[256];
			swprintf_s(text, TEXT("FPS: %f"), fps);
			SetWindowText(displayWin32.hWnd, text);

			frameCount = 0;
		}

		//11 set backbuffer for output
		directXDevice.context->OMSetRenderTargets(1, &directXDevice.rtv, nullptr);

		float color[] = { (std::sin(totalTimeElapsed/2) + 1.0f) / 4.0f, 0.1f, 0.1f, 1.0f };
		directXDevice.context->ClearRenderTargetView(directXDevice.rtv, color);

		for (auto& triangle : triangles)
		{
			triangle.Update(deltaTime, totalTime);
			triangle.Draw(directXDevice.context);
		}

		directXDevice.context->OMSetRenderTargets(0, nullptr, nullptr);

		directXDevice.swapChain->Present(1, /*DXGI_PRESENT_DO_NOT_WAIT*/ 0);
	}

    std::cout << "Hello World!\n";
}

