#include "Game.h"
#include "InputDevice.h"
#include "DisplayWin32.h"
#include <iostream>

Game::Game()
{
	Initialize();
}

void Game::Draw(float deltaTime)
{
	Device.context->OMSetRenderTargets(1, &Device.rtv, nullptr);

	float color[] = { (std::sin(TotalTime / 2) + 1.0f) / 4.0f, 0.1f, 0.1f, 1.0f };
	Device.context->ClearRenderTargetView(Device.rtv, color);

	/*for (auto& triangle : triangles)
	{
		triangle.Update(deltaTime, TotalTime);
		triangle.Draw(Device.context);
	}*/
	for(auto& [key, value] : *Objects)
	{
		value.Update(deltaTime, TotalTime);
		value.Draw(Device.context);
	}
}

void Game::EndFrame()
{
	Device.context->OMSetRenderTargets(0, nullptr, nullptr);

	Device.swapChain->Present(1, /*DXGI_PRESENT_DO_NOT_WAIT*/ 0);
}

void Game::Initialize()
{
	//1 Create a Window
	LPCWSTR applicationName = L"My3DApp";
	Display = new DisplayWin32(applicationName, this);
	Device = DirectXDevice(Display->hWnd, Display->ClientWidth, Display->ClientHeight);
	Input = new InputDevice(this);
	Objects = new std::map<std::string, GameObject>();
	//2 Create Device with the SwapChain
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
	shaderProgram = ShaderProgram(
		Device.device,
		L"./Shaders/MyVeryFirstShader.hlsl",
		inputElements,
		2,
		Shader_Macros
	);
	//10.1 Setup Rasterizer Stage 
	CD3D11_RASTERIZER_DESC rastDesc = {};
	rastDesc.CullMode = D3D11_CULL_NONE;
	rastDesc.FillMode = D3D11_FILL_SOLID;

	auto res = Device.device->CreateRasterizerState(&rastDesc, &rastState);

	Device.context->RSSetState(rastState);

	PrevTime = std::chrono::steady_clock::now();


	GameComponent::Vertex first_triangle[3] = {
		{DirectX::XMFLOAT4(0.1f, 0.2f, 0.1f, 1.0f),	DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
		{DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),	DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
		{DirectX::XMFLOAT4(0.1f, 0.0f, 0.1f, 1.0f),	DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)}
	};

	GameComponent::Vertex second_triangle[3] = {
		{DirectX::XMFLOAT4(0.1f, 0.2f, 0.1f, 1.0f),	DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
		{DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),	DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
		{DirectX::XMFLOAT4(0.0f, 0.2f, 0.0f, 1.0f),	DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)}
	};

	auto triangle1 = std::make_shared<GameComponent>(Device.device, first_triangle);
	auto triangle2 = std::make_shared<GameComponent>(Device.device, second_triangle);



	GameComponent::Vertex third_triangle[3] = {
		{DirectX::XMFLOAT4(0.1f, 0.2f, 0.1f, 1.0f),	DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
		{DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),	DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
		{DirectX::XMFLOAT4(0.1f, 0.0f, 0.1f, 1.0f),	DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)}
	};

	GameComponent::Vertex fourth_triangle[3] = {
		{DirectX::XMFLOAT4(0.1f, 0.2f, 0.1f, 1.0f),	DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
		{DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),	DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
		{DirectX::XMFLOAT4(0.0f, 0.2f, 0.0f, 1.0f),	DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)}
	};


	auto triangle3 = std::make_shared<GameComponent>(Device.device, third_triangle);
	auto triangle4 = std::make_shared<GameComponent>(Device.device, fourth_triangle);
	//GameComponent triangle3(Device.device, third_triangle);
	//GameComponent triangle4(Device.device, fourth_triangle);

	/*triangle3->transform.position.x += 1;
	triangle4->transform.position.x += 1;*/

	//triangles = { triangle1, triangle2 };

	GameObject player_1({ triangle1, triangle2 });
	GameObject player_2({ triangle3, triangle4 });
	//GameObject player_2(std::vector<GameComponent*> {&triangle3, &triangle4});

	Objects->insert({ "player_1", player_1 });
	Objects->insert({ "player_2", player_2 });

	//player_2.move({ 10, 0, 0 });
}

void Game::PrepareFrame()
{
	Device.context->ClearState();

	Device.context->RSSetState(rastState);

	//10.2 Setup ViewPort
	D3D11_VIEWPORT viewport = {};
	viewport.Width = static_cast<float>(Display->ClientWidth);
	viewport.Height = static_cast<float>(Display->ClientHeight);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1.0f;

	Device.context->RSSetViewports(1, &viewport);

	//8 Setup the IA stage
	Device.context->IASetInputLayout(shaderProgram.inputLayout);
	Device.context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//9 Set vertex and pixel shaders
	Device.context->VSSetShader(shaderProgram.vertexShader, nullptr, 0);
	Device.context->PSSetShader(shaderProgram.pixelShader, nullptr, 0);
}

void Game::Update(float deltaTime)
{
	float speed = 0.5f * deltaTime; // скорость движения

	// Игрок 1 (WASD)
	if (Input->IsKeyDown(Keys::W))
	{
		(*Objects)["player_1"].move({ 0, speed, 0 });
		std::cout << (*Objects)["player_1"].visual[0]->transform.position.y << "\n";
	}

	if (Input->IsKeyDown(Keys::S))
	{
		(*Objects)["player_1"].move({ 0, -speed, 0 });
		std::cout << (*Objects)["player_1"].visual[0]->transform.position.y << "\n";
	}

	//// Игрок 2 (стрелки)
	if (Input->IsKeyDown(Keys::Up))
	{
		(*Objects)["player_2"].move({ 0, speed, 0 });
		std::cout << (*Objects)["player_2"].visual[0]->transform.position.y << "\n";
	}

	if (Input->IsKeyDown(Keys::Down))
	{
		(*Objects)["player_2"].move({ 0, -speed, 0 });
		std::cout << (*Objects)["player_2"].visual[0]->transform.position.y << "\n";
	}
	//if (InputDevice->IsKeyDown(Keys::Up))
	//	player2.y += speed;

	//if (InputDevice->IsKeyDown(Keys::Down))
	//	player2.y -= speed;
}

void Game::Run()
{
	PrepareFrame();

	auto	curTime = std::chrono::steady_clock::now();
	float	deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(curTime - PrevTime).count() / 1000000.0f;
	PrevTime = curTime;
	TotalTime += deltaTime;

	TotalTimeForFPS += deltaTime;
	FrameCount++;

	if (TotalTimeForFPS > 1.0f) {
		float fps = FrameCount / TotalTimeForFPS;

		TotalTimeForFPS -= 1.0f;

		WCHAR text[256];
		swprintf_s(text, TEXT("FPS: %f"), fps);
		SetWindowText(Display->hWnd, text);

		FrameCount = 0;
	}
	Update(deltaTime);
	Draw(deltaTime);
	EndFrame();
}
