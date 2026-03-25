#include "Game.h"
#include "InputDevice.h"
#include "DisplayWin32.h"
#include <iostream>

Game::Game()
{
	Initialize();
}
enum Game::GoalResult
{
	None,
	Player1Scored,
	Player2Scored
};
XMFLOAT3 ball_default_velocity = XMFLOAT3(0.5f, 0.0f, 0.0f);

void Game::Draw(float deltaTime)
{
	Device.context->OMSetRenderTargets(1, &Device.rtv, nullptr);

	float color[] = { (std::sin(TotalTime / 2) + 0.1f) / 4.0f, 0.1f, 0.1f, 1.0f };
	Device.context->ClearRenderTargetView(Device.rtv, color);

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
	LPCWSTR applicationName = L"Pong";
	Display = new DisplayWin32(applicationName, this);
	Device = DirectXDevice(Display->hWnd, Display->ClientWidth, Display->ClientHeight);
	Input = new InputDevice(this);
	Objects = new std::map<std::string, GameObject>();
	Leaderboard = new std::map<std::string, int>();
	Leaderboard->insert({ "player_1", 0});
	Leaderboard->insert({ "player_2", 0});
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
	#pragma region Players Creation
	DirectX::XMFLOAT4 color_1 = DirectX::XMFLOAT4(0.46f, 0.65f, 0.26f, 1.0f);
	GameComponent::Vertex first_triangle[3] = {
	{DirectX::XMFLOAT4(0.05f, 0.1f, 0.1f, 1.0f),	color_1},
	{DirectX::XMFLOAT4(-0.05f, -0.1f, 0.0f, 1.0f),	color_1},
	{DirectX::XMFLOAT4(0.05f, -0.1f, 0.1f, 1.0f),	color_1}
	};

	GameComponent::Vertex second_triangle[3] = {
		{DirectX::XMFLOAT4(0.05f, 0.1f, 0.1f, 1.0f),	color_1},
		{DirectX::XMFLOAT4(-0.05f, -0.1f, 0.0f, 1.0f),	color_1},
		{DirectX::XMFLOAT4(-0.05f, 0.1f, 0.0f, 1.0f),	color_1}
	};

	DirectX::XMFLOAT4 color_2 = DirectX::XMFLOAT4(0.31f, 0.56f, 0.73f, 1.0f);
	GameComponent::Vertex third_triangle[3] = {
	{DirectX::XMFLOAT4(0.05f, 0.1f, 0.1f, 1.0f),	color_2},
	{DirectX::XMFLOAT4(-0.05f, -0.1f, 0.0f, 1.0f),	color_2},
	{DirectX::XMFLOAT4(0.05f, -0.1f, 0.1f, 1.0f),	color_2}
	};

	GameComponent::Vertex fourth_triangle[3] = {
		{DirectX::XMFLOAT4(0.05f, 0.1f, 0.1f, 1.0f),	color_2},
		{DirectX::XMFLOAT4(-0.05f, -0.1f, 0.0f, 1.0f),	color_2},
		{DirectX::XMFLOAT4(-0.05f, 0.1f, 0.0f, 1.0f),	color_2}
	};

	auto triangle1 = std::make_shared<GameComponent>(Device.device, first_triangle);
	auto triangle2 = std::make_shared<GameComponent>(Device.device, second_triangle);
	auto triangle3 = std::make_shared<GameComponent>(Device.device, third_triangle);
	auto triangle4 = std::make_shared<GameComponent>(Device.device, fourth_triangle);

	GameObject player_1({ triangle1, triangle2 });
	GameObject player_2({ triangle3, triangle4 });
	player_1.physics = PhysicsComponent(XMFLOAT3(0.0f, 0.0f, 0.0f), { 0.05f, 0.1f });
	player_2.physics = PhysicsComponent(XMFLOAT3(0.0f, 0.0f, 0.0f), { 0.05f, 0.1f });
	player_1.physics.mass = 1.0f;
	player_2.physics.mass = 1.0f;
	player_1.UpdateBoundingBox();
	player_2.UpdateBoundingBox();
	player_1.move({ -0.75f, 0, 0 });
	player_2.move({ 0.75f, 0, 0 });
	Objects->insert({ "player_1", player_1 });
	Objects->insert({ "player_2", player_2 });
	#pragma endregion

	#pragma region Ball Creation
	DirectX::XMFLOAT4 color_3 = DirectX::XMFLOAT4(0.65f, 0.19f, 0.19f, 1.0f);

	std::vector<std::shared_ptr<GameComponent>> ball_components;
	float radius = 0.05f;
	int sides = 16;

	for (int i = 0; i < sides; i++) {
		float angle1 = 2.0f * 3.14f * i / sides;
		float angle2 = 2.0f * 3.14f * (i + 1) / sides;

		float x1 = radius * cos(angle1);
		float z1 = radius * sin(angle1);
		float x2 = radius * cos(angle2);
		float z2 = radius * sin(angle2);

		GameComponent::Vertex triangle[3] = {
			{DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), color_3},
			{DirectX::XMFLOAT4(x1, z1, 0, 1.0f), color_3},
			{DirectX::XMFLOAT4(x2, z2, 0, 1.0f), color_3}
		};
		ball_components.push_back(std::make_shared<GameComponent>(Device.device, triangle));
	}

	GameObject ball(ball_components);
	ball.physics = PhysicsComponent(ball_default_velocity, { 0.05f, 0.05f });
	ball.physics.mass = 1.0f;
	ball.UpdateBoundingBox();

	Objects->insert({ "ball", ball });
	#pragma endregion

	#pragma region Walls Creation
	DirectX::XMFLOAT4 color_4 = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	GameComponent::Vertex first_wall[3] = {
		{DirectX::XMFLOAT4(2.05f, 0.1f, 0.1f, 1.0f),	color_4},
		{DirectX::XMFLOAT4(-2.05f, -0.1f, 0.0f, 1.0f),	color_4},
		{DirectX::XMFLOAT4(2.05f, -0.1f, 0.1f, 1.0f),	color_4}
	};

	GameComponent::Vertex second_wall[3] = {
		{DirectX::XMFLOAT4(2.05f, 0.1f, 0.1f, 1.0f),	color_4},
		{DirectX::XMFLOAT4(-2.05f, -0.1f, 0.0f, 1.0f),	color_4},
		{DirectX::XMFLOAT4(-2.05f, 0.1f, 0.0f, 1.0f),	color_4}
	};
	auto wall1 = std::make_shared<GameComponent>(Device.device, first_wall);
	auto wall2 = std::make_shared<GameComponent>(Device.device, second_wall);
	auto wall3 = std::make_shared<GameComponent>(Device.device, first_wall);
	auto wall4 = std::make_shared<GameComponent>(Device.device, second_wall);
	GameObject wall_1({ wall1, wall2 });
	GameObject wall_2({ wall3, wall4 });
	wall_1.move({ 0, 0.60f, 0 });
	wall_2.move({ 0, -0.60f, 0 });
	Objects->insert({ "wall_1", wall_1 });
	Objects->insert({ "wall_2", wall_2 });
	#pragma endregion
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
	}

	if (Input->IsKeyDown(Keys::S))
	{
		(*Objects)["player_1"].move({ 0, -speed, 0 });
	}
	ClampPlayerY((*Objects)["player_1"], 0.5f, -0.5f, speed);

	// Игрок 2 (стрелки)
	if (Input->IsKeyDown(Keys::Up))
	{
		(*Objects)["player_2"].move({ 0, speed, 0 });
	}

	if (Input->IsKeyDown(Keys::Down))
	{
		(*Objects)["player_2"].move({ 0, -speed, 0 });
	}
	ClampPlayerY((*Objects)["player_2"], 0.5f, -0.5f, speed);

	if ((*Objects)["ball"].CheckCollision((*Objects)["player_2"]))
	{
		(*Objects)["ball"].ResolveCollision((*Objects)["player_2"], deltaTime);
	}

	if ((*Objects)["ball"].CheckCollision((*Objects)["player_1"]))
	{
		(*Objects)["ball"].ResolveCollision((*Objects)["player_1"], deltaTime);
	}
	(*Objects)["ball"].HandleWallCollision(0.5f, -0.5f);
	auto result = CheckGoal((*Objects)["ball"], -0.8f, 0.8f);
	if (result == GoalResult::Player1Scored)
	{
		(*Objects)["ball"].move_teleport({ 0,0,0 });
		(*Objects)["ball"].physics.velocity = ball_default_velocity;
		(*Leaderboard)["player_1"] += 1;
		std::cout << "Goal by Player 1.   " << (*Leaderboard)["player_1"] << ":" << (*Leaderboard)["player_2"] << "\n";
	};
	if (result == GoalResult::Player2Scored)
	{
		(*Objects)["ball"].move_teleport({ 0,0,0 });
		(*Objects)["ball"].physics.velocity = ball_default_velocity;
		(*Leaderboard)["player_2"] += 1;
		std::cout << "Goal by Player 2.   " << (*Leaderboard)["player_1"] << ":" << (*Leaderboard)["player_2"] << "\n";
	};

}

void Game::ClampPlayerY(GameObject& player, float topBound, float bottomBound, float speed)
{
	float& y = player.position.y;
	float halfHeight = player.physics.size.y;

	if (y + halfHeight > topBound)
	{
		player.move({ 0,-speed,0 });
	}

	if (y - halfHeight < bottomBound)
	{
		player.move({ 0,speed,0 });
	}

	player.UpdateBoundingBox();
}

Game::GoalResult Game::CheckGoal(GameObject& ball, float leftBound, float rightBound)
{
	float ballX = ball.position.x;
	float halfWidth = ball.physics.size.x;

	if (ballX + halfWidth < leftBound)
	{
		return GoalResult::Player2Scored;
	}

	if (ballX - halfWidth > rightBound)
	{
		return GoalResult::Player1Scored;
	}

	return GoalResult::None;
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
