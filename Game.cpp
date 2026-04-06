#include "Game.h"
#include "InputDevice.h"
#include "DisplayWin32.h"
#include <iostream>
#include <algorithm>
#include "ModelLoader.h"
ID3D11Texture2D* depthStencilBuffer = nullptr;
ID3D11DepthStencilView* depthStencilView = nullptr;
bool orbiting_camera = true;
bool ortho_camera = false;
float input_cooldown = 0.0f;
std::vector<std::shared_ptr<GameObject>> ObjectsToCheckForCollision{};
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
	cb.time = TotalTime;

	Device.context->UpdateSubresource(cameraCB, 0, nullptr, &cb, 0, 0);
	Device.context->VSSetConstantBuffers(0, 1, &cameraCB);





	Device.context->OMSetRenderTargets(1, &Device.rtv, depthStencilView);

	float color[] = { (std::sin(TotalTime / 2) + 0.1f) / 4.0f, 0.1f, 0.1f, 1.0f };
	Device.context->ClearRenderTargetView(Device.rtv, color);
	Device.context->ClearDepthStencilView(
		depthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0
	);

	for (auto& [key, value] : *Objects)
	{
		value->Update(deltaTime, TotalTime);

		XMMATRIX scaleM = XMMatrixScaling(
			value->transform.scale.x,
			value->transform.scale.y,
			value->transform.scale.z
		);
		XMVECTOR q = XMLoadFloat4(&value->transform.rotation);
		XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(q);
		XMMATRIX translationMatrix = XMMatrixTranslation(
			value->transform.position.x,
			value->transform.position.y,
			value->transform.position.z
		);
		XMMATRIX world = scaleM * rotationMatrix * translationMatrix;
		cb.world = XMMatrixTranspose(world);
		XMFLOAT3 modelOffset = value->visual->transform.offset;
		cb.modelOffset = modelOffset;

		Device.context->UpdateSubresource(cameraCB, 0, nullptr, &cb, 0, 0);

		value->Draw(Device.context);
	}
}

void Game::EndFrame()
{
	Device.context->OMSetRenderTargets(0, nullptr, nullptr);

	Device.swapChain->Present(1, /*DXGI_PRESENT_DO_NOT_WAIT*/ 0);
}

GameComponent::MeshData CreateBox(float width, float height, float depth, DirectX::XMFLOAT4 color)
{
	using Vertex = GameComponent::Vertex;
	GameComponent::MeshData mesh;

	float hw = width * 0.5f;
	float hh = height * 0.5f;
	float hd = depth * 0.5f;

	// 1. Создаём вершины (каждая грань имеет свои нормали)
	// Порядок: front, back, top, bottom, left, right

	// FRONT (+Z)
	mesh.vertices.push_back({ { -hw, -hh,  hd, 1 }, { 0, 0, 1 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw, -hh,  hd, 1 }, { 0, 0, 1 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw,  hh,  hd, 1 }, { 0, 0, 1 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw,  hh,  hd, 1 }, { 0, 0, 1 }, {1,1}, color });

	// BACK (-Z)
	mesh.vertices.push_back({ {  hw, -hh, -hd, 1 }, { 0, 0, -1 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw, -hh, -hd, 1 }, { 0, 0, -1 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw,  hh, -hd, 1 }, { 0, 0, -1 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw,  hh, -hd, 1 }, { 0, 0, -1 }, {1,1}, color });

	// TOP (+Y)
	mesh.vertices.push_back({ { -hw,  hh,  hd, 1 }, { 0, 1, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw,  hh,  hd, 1 }, { 0, 1, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw,  hh, -hd, 1 }, { 0, 1, 0 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw,  hh, -hd, 1 }, { 0, 1, 0 }, {1,1}, color });

	// BOTTOM (-Y)
	mesh.vertices.push_back({ { -hw, -hh, -hd, 1 }, { 0, -1, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw, -hh, -hd, 1 }, { 0, -1, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw, -hh,  hd, 1 }, { 0, -1, 0 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw, -hh,  hd, 1 }, { 0, -1, 0 }, {1,1}, color });

	// LEFT (-X)
	mesh.vertices.push_back({ { -hw, -hh, -hd, 1 }, { -1, 0, 0 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw, -hh,  hd, 1 }, { -1, 0, 0 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw,  hh,  hd, 1 }, { -1, 0, 0 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw,  hh, -hd, 1 }, { -1, 0, 0 }, {1,1}, color });

	// RIGHT (+X)
	mesh.vertices.push_back({ {  hw, -hh,  hd, 1 }, { 1, 0, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw, -hh, -hd, 1 }, { 1, 0, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw,  hh, -hd, 1 }, { 1, 0, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw,  hh,  hd, 1 }, { 1, 0, 0 }, {1,1}, color });

	// 2. Индексы для каждой грани (2 треугольника на грань)
	for (int i = 0; i < 6; ++i)
	{
		uint32_t start = i * 4;
		mesh.indices.push_back(start + 0);
		mesh.indices.push_back(start + 1);
		mesh.indices.push_back(start + 2);

		mesh.indices.push_back(start + 0);
		mesh.indices.push_back(start + 2);
		mesh.indices.push_back(start + 3);
	}

	return mesh;
}

GameComponent::MeshData CreateSphere(float radius, int sliceCount, int stackCount, DirectX::XMFLOAT4 color)
{
	using Vertex = GameComponent::Vertex;
	GameComponent::MeshData mesh;

	// 1. Создаём сетку вершин
	for (int i = 0; i <= stackCount; ++i)
	{
		float phi = XM_PI * i / stackCount; // от 0 до PI

		for (int j = 0; j <= sliceCount; ++j)
		{
			float theta = 2.0f * XM_PI * j / sliceCount; // от 0 до 2PI

			float x = radius * sinf(phi) * cosf(theta);
			float y = radius * cosf(phi);
			float z = radius * sinf(phi) * sinf(theta);

			XMFLOAT4 pos = { x, y, z, 1 };

			// нормаль для smooth shading
			//XMVECTOR n = XMVector3Normalize(XMLoadFloat4(&pos));
			XMVECTOR n = XMVector3Normalize(XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(&pos)));
			XMFLOAT3 normal;
			XMStoreFloat3(&normal, n);

			mesh.vertices.push_back({ pos, normal, {1,1}, color });
		}
	}

	// 2. Создаём индексный буфер
	for (int i = 0; i < stackCount; ++i)
	{
		for (int j = 0; j < sliceCount; ++j)
		{
			int first = i * (sliceCount + 1) + j;
			int second = first + sliceCount + 1;

			mesh.indices.push_back(first);
			mesh.indices.push_back(second);
			mesh.indices.push_back(first + 1);

			mesh.indices.push_back(first + 1);
			mesh.indices.push_back(second);
			mesh.indices.push_back(second + 1);
		}
	}
	
	return mesh;
}

void Game::CreateOrbitingCube(std::string name, DirectX::XMFLOAT4 color, float size, std::shared_ptr<GameObject> target, XMFLOAT3 planetOffset, XMFLOAT3 rotationAxis, float speed, float selfspeed = XM_PIDIV2)
{
	auto boxMesh = CreateBox(size, size, size, color);
	auto boxComponent = std::make_shared<GameComponent>(Device.device, boxMesh.vertices, boxMesh.indices);
	Objects->insert({ name, std::make_shared<OrbitObject>(boxComponent, target, planetOffset, rotationAxis, speed, selfspeed) });
}

void Game::CreateOrbitingSphere(std::string name, DirectX::XMFLOAT4 color, float size, std::shared_ptr<GameObject> target, XMFLOAT3 planetOffset, XMFLOAT3 rotationAxis, float speed, float selfspeed = XM_PIDIV2)
{
	auto sphereMesh = CreateSphere(size, 16, 16, color);
	auto sphereComponent = std::make_shared<GameComponent>(Device.device, sphereMesh.vertices, sphereMesh.indices);
	Objects->insert({ name, std::make_shared<OrbitObject>(sphereComponent, target, planetOffset, rotationAxis, speed, selfspeed) });
}

std::shared_ptr<GameObject> Game::GetGameObject(std::string name)
{
	return Objects->find(name)->second;
}

void Game::CreateModelObject(std::string name, std::string model_path, XMFLOAT3 collision_size)
{
	auto mesh = ModelLoader().LoadModel(Device.device, Device.context, model_path);

	auto component = std::make_shared<GameComponent>(
		Device.device,
		mesh.vertices,
		mesh.indices,
		mesh.texture
	);
	auto obj = std::make_shared<GameObject>(component, collision_size);
	obj->InitBoundingBoxBuffers(Device.device);
	Objects->insert({ name, obj });
}

void Game::CreateObjects()
{
	/*CreateOrbitingSphere("sun", DirectX::XMFLOAT4(1, 1, 0, 1), 1.5f, nullptr, { 3,0,0 }, { 0,1,0 }, XM_PI/8);
	CreateOrbitingSphere("merc", DirectX::XMFLOAT4(0.81f, 0.86f, 0.60f, 1), 0.1f, Objects->find("sun")->second, { 3,0,0 }, { 0,1,0 }, XM_PI/16);
	CreateOrbitingSphere("ven", DirectX::XMFLOAT4(0.86f, 0.55f, 0.1f, 1), 0.2f, Objects->find("sun")->second, { 4.0f,0,0 }, { 0,1,0 }, XM_PI/14);

	CreateOrbitingSphere("earth", DirectX::XMFLOAT4(0.43f, 0.75f, 0.76f, 1), 0.4f, Objects->find("sun")->second, { 6,0,0 }, { 0,1,0 }, XM_PI/18, 4);
	CreateOrbitingCube("moon", DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1), 0.2f, Objects->find("earth")->second, { 1,0,0 }, { 0,1,0 }, XM_PIDIV2);
	CreateOrbitingSphere("mars", DirectX::XMFLOAT4(0.97f, 0.4f, 0.2f, 1), 0.3f, Objects->find("sun")->second, { 8.0f,0,0 }, { 0,1,0 }, XM_PI/6);
	CreateOrbitingCube("mars_moon", DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1), 0.15f, Objects->find("mars")->second, { 0.4f,0,0 }, { 0,1,0 }, XM_PIDIV2, 6);
	CreateOrbitingCube("mars_moon2", DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1), 0.15f, Objects->find("mars")->second, { 0.6f,0,0 }, { 0,1,0 }, XM_PI/3);

	CreateOrbitingSphere("jupiter", DirectX::XMFLOAT4(0.8f, 0.5f, 0.5f, 1), 0.8f, Objects->find("sun")->second, { 10,0,0 }, { 0,1,0 }, XM_PI/8);
	CreateOrbitingSphere("saturn", DirectX::XMFLOAT4(0.8f, 0.76f, 0.5f, 1), 0.7f, Objects->find("sun")->second, { 12,0,0 }, { 0,1,0 }, XM_PI/9);
	CreateOrbitingSphere("uran", DirectX::XMFLOAT4(0.43f, 0.75f, 0.76f, 1), 0.4f, Objects->find("sun")->second, { 14,0,0 }, { 0,1,0 }, XM_PI / 7);
	CreateOrbitingSphere("neptun", DirectX::XMFLOAT4(0.53f, 0.75f, 0.96f, 1), 0.6f, Objects->find("sun")->second, { 16,0,0 }, { 0,1,0 }, XM_PI / 7.5);*/
	CreateModelObject("ball", "Models/beach_ball.glb", XMFLOAT3{ 10.5f,10,10.1f });
	GetGameObject("ball")->SetScale(0.1f);
	CreateModelObject("toilet", "Models/shrek_toilet.glb", XMFLOAT3 {0.5f,1,0.1f});
	GetGameObject("toilet")->visual->transform.offset = XMFLOAT3{ -0, -1, -0.5 };
	GetGameObject("toilet")->SetScale(2.15f);
	GetGameObject("toilet")->move({ 5.15f, 0, 0 });
	//GetGameObject("toilet")->move({ 5.15f, 2, 0 });
	ObjectsToCheckForCollision.push_back(GetGameObject("toilet"));
	GetGameObject("toilet")->localPosition = GetGameObject("toilet")->transform.position;
	GetGameObject("toilet")->localRotation = GetGameObject("toilet")->transform.rotation;
	GetGameObject("toilet")->localScale = GetGameObject("toilet")->transform.scale;
	GetGameObject("toilet")->parent = GetGameObject("ball");

}

void Game::ChangeMouseModeToFPS()
{
	Input->MouseMove.RemoveAll();
	Input->MouseMove.AddLambda([this](const InputDevice::MouseMoveEventArgs& args)
		{
			float sensitivity = 0.002f;

			mCam.RotateY(args.Offset.x * sensitivity);
			mCam.Pitch(args.Offset.y * sensitivity);
		});
}
void Game::ChangeMouseModeToOrbiting()
{
	Input->MouseMove.RemoveAll();
	Input->MouseMove.AddLambda([this](const InputDevice::MouseMoveEventArgs& args)
		{
			float sensitivity = -0.002f;

			mCam.mTheta -= args.Offset.x * sensitivity;
			mCam.mPhi += args.Offset.y * sensitivity;
			mCam.mRadius -= args.WheelDelta * 0.001f;
			mCam.mRadius = std::clamp(mCam.mRadius, 10.0f, 50.0f);
		});
}

void Game::ChangeCameraModeToOrthographic()
{
	mCam.SetOrthoLens(14, 14, 0.1f, 100.0f);
}

void Game::ChangeCameraModeToPerspective()
{
	mCam.SetLens(
		0.25f * DirectX::XM_PI,
		(float)Display->ClientWidth / Display->ClientHeight,
		0.1f,
		100.0f
	);
}
void Game::Initialize()
{
	//1 Create a Window
	LPCWSTR applicationName = L"Planets";
	Display = new DisplayWin32(applicationName, this);
	Device = DirectXDevice(Display->hWnd, Display->ClientWidth, Display->ClientHeight);
	Input = new InputDevice(this);
	Objects = new std::map<std::string, std::shared_ptr<GameObject>>();

	//mCam.SetPosition(0.0f, 0.0f, -82.0f);

	mCam.LookAt(
		DirectX::XMVectorSet(0, 0, -22, 1),
		DirectX::XMVectorZero(),
		DirectX::XMVectorSet(0, 1, 0, 0)
	);

	mCam.SetLens(
		0.25f * DirectX::XM_PI,
		(float)Display->ClientWidth / Display->ClientHeight,
		0.1f,
		100.0f
	);
	//mCam.SetOrthoLens(2, 2, 0.1f, 100.0f);



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

	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{
		"TEXCOORD",
		0,
		DXGI_FORMAT_R32G32_FLOAT,
		0,
		D3D11_APPEND_ALIGNED_ELEMENT,
		D3D11_INPUT_PER_VERTEX_DATA,
		0
	},
	{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	
	};
	D3D_SHADER_MACRO Shader_Macros[] = { "TEST", "1", "TCOLOR", "float4(0.0f, 1.0f, 0.0f, 1.0f)", nullptr, nullptr };
	shaderProgram = ShaderProgram(
		Device.device,
		L"./Shaders/MyVeryFirstShader.hlsl",
		inputElements,
		4,
		Shader_Macros
	);
	//10.1 Setup Rasterizer Stage 
	CD3D11_RASTERIZER_DESC rastDesc = {};
	rastDesc.CullMode = D3D11_CULL_NONE;
	rastDesc.FillMode = D3D11_FILL_SOLID;

	auto res = Device.device->CreateRasterizerState(&rastDesc, &rastState);

	Device.context->RSSetState(rastState);

	PrevTime = std::chrono::steady_clock::now();

	CreateObjects();

	ChangeCameraModeToPerspective();
	ChangeMouseModeToOrbiting();

	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width = Display->ClientWidth;
	depthDesc.Height = Display->ClientHeight;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	Device.device->CreateTexture2D(&depthDesc, nullptr, &depthStencilBuffer);
	Device.device->CreateDepthStencilView(depthStencilBuffer, nullptr, &depthStencilView);

	ID3D11SamplerState* sampler = nullptr;

	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	Device.device->CreateSamplerState(&sampDesc, &sampler);
	Device.context->PSSetSamplers(0, 1, &sampler);
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
	input_cooldown += deltaTime;
	//GetGameObject("ball")->move({ 1*deltaTime,0,0 });
	if (orbiting_camera)
	{
		mCam.mTarget = GetGameObject("ball")->transform.position;
		mCam.UpdateOrbit();

		XMFLOAT3 inputDir{ 0,0,0 };
		XMFLOAT3 velocity{ 0,0,0 };
		float speed = 5.0f;
		if (Input->IsKeyDown(Keys::W)) inputDir.z += 1.0f;
		if (Input->IsKeyDown(Keys::S)) inputDir.z -= 1.0f;
		if (Input->IsKeyDown(Keys::A)) inputDir.x -= 1.0f;
		if (Input->IsKeyDown(Keys::D)) inputDir.x += 1.0f;
		velocity.x = inputDir.x * speed * deltaTime;
		velocity.z = inputDir.z * speed * deltaTime;

		GetGameObject("ball")->move(velocity);
		XMFLOAT3 axis{ velocity.z, 0.0f, -velocity.x };
		float length = sqrtf(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
		if (length > 0)
		{
			axis.x /= length;
			axis.y /= length;
			axis.z /= length;

			float distance = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
			float angle = distance / 3.0f;

			GetGameObject("ball")->Rotate(axis, angle);
		}
	}

	if (!orbiting_camera)
	{
		if (Input->IsKeyDown(Keys::W))
		{
			mCam.Walk(10.0f * deltaTime);
		}

		if (Input->IsKeyDown(Keys::S))
		{
			mCam.Walk(-10.0f * deltaTime);
		}
		if (Input->IsKeyDown(Keys::A))
		{
			mCam.Strafe(-10.0f * deltaTime);
		}

		if (Input->IsKeyDown(Keys::D))
		{
			mCam.Strafe(10.0f * deltaTime);
		}
	}
	
	if (input_cooldown > 1.0f)
	{
		if (Input->IsKeyDown(Keys::E))
		{
			input_cooldown = 0;
			orbiting_camera = !orbiting_camera;
			if (orbiting_camera)
				ChangeMouseModeToOrbiting();
			else
				ChangeMouseModeToFPS();
		}
		if (Input->IsKeyDown(Keys::Q))
		{
			input_cooldown = 0;
			ortho_camera = !ortho_camera;
			if (ortho_camera)
				ChangeCameraModeToOrthographic();
			else
				ChangeCameraModeToPerspective();
		}
	}
	for (size_t i = 0; i < ObjectsToCheckForCollision.size(); i++)
	{
		if (GetGameObject("ball")->CheckCollision(ObjectsToCheckForCollision[i])) {

		}
	}
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
