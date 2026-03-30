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
	mCam.UpdateViewMatrix();
	CameraBuffer cb;
	cb.view = DirectX::XMMatrixTranspose(mCam.View());
	cb.proj = DirectX::XMMatrixTranspose(mCam.Proj());
	//cb.viewProj = DirectX::XMMatrixTranspose(mCam.ViewProj());
	//cb.world = XMMatrixTranspose(XMMatrixIdentity());

	Device.context->UpdateSubresource(cameraCB, 0, nullptr, &cb, 0, 0);
	Device.context->VSSetConstantBuffers(0, 1, &cameraCB);






	Device.context->OMSetRenderTargets(1, &Device.rtv, nullptr);

	float color[] = { (std::sin(TotalTime / 2) + 0.1f) / 4.0f, 0.1f, 0.1f, 1.0f };
	Device.context->ClearRenderTargetView(Device.rtv, color);

	for (auto& [key, value] : *Objects)
	{
		value.Update(deltaTime, TotalTime);
		for (auto& component : value.visual)
		{
			XMMATRIX world = XMMatrixTranslation(
				value.position.x,
				value.position.y,
				value.position.z
			);

			cb.world = XMMatrixTranspose(world);


			Device.context->UpdateSubresource(cameraCB, 0, nullptr, &cb, 0, 0);

			component->Draw(Device.context);
		}
	}

	/*for(auto& [key, value] : *Objects)
	{
		value.Update(deltaTime, TotalTime);
		value.Draw(Device.context);
	}*/
}

void Game::EndFrame()
{
	Device.context->OMSetRenderTargets(0, nullptr, nullptr);

	Device.swapChain->Present(1, /*DXGI_PRESENT_DO_NOT_WAIT*/ 0);
}

std::vector<GameComponent::Vertex> CreateBox(float w, float h, float d)
{
	float hw = w * 0.5f;
	float hh = h * 0.5f;
	float hd = d * 0.5f;

	using V = GameComponent::Vertex;

	// 8 вершин куба (каждая со своим цветом)
	V v[8] =
	{
		{{-hw,-hh,-hd,1}, {1,0,0,1}}, // 0
		{{-hw, hh,-hd,1}, {0,1,0,1}}, // 1
		{{ hw, hh,-hd,1}, {0,0,1,1}}, // 2
		{{ hw,-hh,-hd,1}, {1,1,0,1}}, // 3
		{{-hw,-hh, hd,1}, {1,0,1,1}}, // 4
		{{-hw, hh, hd,1}, {0,1,1,1}}, // 5
		{{ hw, hh, hd,1}, {1,1,1,1}}, // 6
		{{ hw,-hh, hd,1}, {0,0,0,1}}, // 7
	};

	std::vector<V> vertices;

	auto addTri = [&](int a, int b, int c)
		{
			vertices.push_back(v[a]);
			vertices.push_back(v[b]);
			vertices.push_back(v[c]);
		};

	// 12 треугольников (6 граней)

	// front
	addTri(4, 5, 6);
	addTri(4, 6, 7);

	// back
	addTri(0, 2, 1);
	addTri(0, 3, 2);

	// left
	addTri(0, 1, 5);
	addTri(0, 5, 4);

	// right
	addTri(3, 7, 6);
	addTri(3, 6, 2);

	// top
	addTri(1, 2, 6);
	addTri(1, 6, 5);

	// bottom
	addTri(0, 4, 7);
	addTri(0, 7, 3);

	return vertices;
}

void Game::Initialize()
{
	//1 Create a Window
	LPCWSTR applicationName = L"Planets";
	Display = new DisplayWin32(applicationName, this);
	Device = DirectXDevice(Display->hWnd, Display->ClientWidth, Display->ClientHeight);
	Input = new InputDevice(this);
	Objects = new std::map<std::string, GameObject>();
	Leaderboard = new std::map<std::string, int>();
	Leaderboard->insert({ "player_1", 0});
	Leaderboard->insert({ "player_2", 0});

	mCam.SetPosition(0.0f, 0.0f, -2.0f);

	mCam.LookAt(
		DirectX::XMVectorSet(0, 0, -2, 1),
		DirectX::XMVectorZero(),
		DirectX::XMVectorSet(0, 1, 0, 0)
	);

	mCam.SetLens(
		0.25f * DirectX::XM_PI,
		(float)Display->ClientWidth / Display->ClientHeight,
		0.1f,
		100.0f
	);



	D3D11_BUFFER_DESC cbd = {};
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(CameraBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	Device.device->CreateBuffer(&cbd, nullptr, &cameraCB);



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

	GameComponent::Vertex ball_1[3] = {
		{DirectX::XMFLOAT4(0.05f, 0.05f, 0.1f, 1.0f),	color_3},
		{DirectX::XMFLOAT4(-0.05f, -0.05f, 0.0f, 1.0f),	color_3},
		{DirectX::XMFLOAT4(0.05f, -0.05f, 0.1f, 1.0f),	color_3}
	};
	GameComponent::Vertex ball_2[3] = {
		{DirectX::XMFLOAT4(0.05f, 0.05f, 0.1f, 1.0f),	color_3},
		{DirectX::XMFLOAT4(-0.05f, -0.05f, 0.0f, 1.0f),	color_3},
		{DirectX::XMFLOAT4(-0.05f, 0.05f, 0.1f, 1.0f),	color_3}
	};
	auto ball1 = std::make_shared<GameComponent>(Device.device, ball_1);
	auto ball2 = std::make_shared<GameComponent>(Device.device, ball_2);
	GameObject ball({ ball1, ball2 });
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



	auto verts = CreateBox(0.3f, 0.3f, 0.3f);

	std::vector<std::shared_ptr<GameComponent>> components;

	for (size_t i = 0; i < verts.size(); i += 3)
	{
		GameComponent::Vertex tri[3] =
		{
			verts[i],
			verts[i + 1],
			verts[i + 2]
		};

		components.push_back(std::make_shared<GameComponent>(Device.device, tri));
	}
	GameObject box(components);
	box.move({ 0, 0, 0 });

	Objects->insert({ "box", box });

	Input->MouseMove.AddLambda([this](const InputDevice::MouseMoveEventArgs& args)
		{
			float sensitivity = 0.002f;

			mCam.RotateY(args.Offset.x * sensitivity); // горизонт
			mCam.Pitch(args.Offset.y * sensitivity);   // вертикаль
		});
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
	if (Input->IsKeyDown(Keys::W))
	{
		mCam.Walk(10.0f * deltaTime);
	}

	if (Input->IsKeyDown(Keys::S))
	{
		mCam.Walk(-10.0f * deltaTime);
		std::cout << mCam.GetPosition().z << "\n";
	}
	if (Input->IsKeyDown(Keys::A))
	{
		mCam.Strafe(-10.0f * deltaTime);
	}

	if (Input->IsKeyDown(Keys::D))
	{
		mCam.Strafe(10.0f * deltaTime);
	}








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
