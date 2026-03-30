#include "Game.h"
#include "InputDevice.h"
#include "DisplayWin32.h"
#include <iostream>
#include <algorithm>
ID3D11Texture2D* depthStencilBuffer = nullptr;
ID3D11DepthStencilView* depthStencilView = nullptr;
bool orbiting_camera = false;
bool ortho_camera = false;
float input_cooldown = 0.0f;
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





	Device.context->OMSetRenderTargets(1, &Device.rtv, depthStencilView);
	//Device.context->OMSetRenderTargets(1, &Device.rtv, nullptr);

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
		for (auto& component : value->visual)
		{
			XMVECTOR q = XMLoadFloat4(&value->rotation);
			XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(q);
			XMMATRIX translationMatrix = XMMatrixTranslation(
				value->position.x,
				value->position.y,
				value->position.z
			);
			XMMATRIX world = rotationMatrix * translationMatrix;
			cb.world = XMMatrixTranspose(world);


			Device.context->UpdateSubresource(cameraCB, 0, nullptr, &cb, 0, 0);

			component->Draw(Device.context);
		}
	}
}

void Game::EndFrame()
{
	Device.context->OMSetRenderTargets(0, nullptr, nullptr);

	Device.swapChain->Present(1, /*DXGI_PRESENT_DO_NOT_WAIT*/ 0);
}

std::vector<GameComponent::Vertex> CreateBox(float w, float h, float d, XMFLOAT4 color)
{
	float hw = w * 0.5f;
	float hh = h * 0.5f;
	float hd = d * 0.5f;

	using V = GameComponent::Vertex;

	V v[8] =
	{
		{{-hw,-hh,-hd,1}, color},
		{{-hw, hh,-hd,1}, color},
		{{ hw, hh,-hd,1}, color},
		{{ hw,-hh,-hd,1}, color},
		{{-hw,-hh, hd,1}, color},
		{{-hw, hh, hd,1}, color},
		{{ hw, hh, hd,1}, color},
		{{ hw,-hh, hd,1}, color},
	};

	std::vector<V> vertices;

	auto addTri = [&](int a, int b, int c)
		{
			vertices.push_back(v[a]);
			vertices.push_back(v[b]);
			vertices.push_back(v[c]);
		};

	addTri(4, 5, 6);
	addTri(4, 6, 7);

	addTri(0, 2, 1);
	addTri(0, 3, 2);

	addTri(0, 1, 5);
	addTri(0, 5, 4);

	addTri(3, 7, 6);
	addTri(3, 6, 2);

	addTri(1, 2, 6);
	addTri(1, 6, 5);

	addTri(0, 4, 7);
	addTri(0, 7, 3);

	return vertices;
}

std::vector<GameComponent::Vertex> CreateSphere(
	float radius,
	int sliceCount, 
	int stackCount, 
	DirectX::XMFLOAT4 color
)
{
	using V = GameComponent::Vertex;
	std::vector<V> vertices;

	std::vector<V> grid;

	for (int i = 0; i <= stackCount; ++i)
	{
		float phi = DirectX::XM_PI * i / stackCount;

		for (int j = 0; j <= sliceCount; ++j)
		{
			float theta = 2.0f * DirectX::XM_PI * j / sliceCount;

			float x = radius * sinf(phi) * cosf(theta);
			float y = radius * cosf(phi);
			float z = radius * sinf(phi) * sinf(theta);

			grid.push_back({ {x, y, z, 1.0f}, color });
		}
	}

	for (int i = 0; i < stackCount; ++i)
	{
		for (int j = 0; j < sliceCount; ++j)
		{
			int a = i * (sliceCount + 1) + j;
			int b = a + sliceCount + 1;

			vertices.push_back(grid[a]);
			vertices.push_back(grid[b]);
			vertices.push_back(grid[a + 1]);

			vertices.push_back(grid[a + 1]);
			vertices.push_back(grid[b]);
			vertices.push_back(grid[b + 1]);
		}
	}

	return vertices;
}

void Game::CreateOrbitingCube(std::string name, DirectX::XMFLOAT4 color, float size, std::shared_ptr<GameObject> target, XMFLOAT3 planetOffset, XMFLOAT3 rotationAxis, float speed, float selfspeed = XM_PIDIV2)
{
	auto verts = CreateBox(size, size, size, color);

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
	Objects->insert({ name, std::make_shared<OrbitObject>(components, target, planetOffset, rotationAxis, speed, selfspeed) });
}

void Game::CreateOrbitingSphere(std::string name, DirectX::XMFLOAT4 color, float size, std::shared_ptr<GameObject> target, XMFLOAT3 planetOffset, XMFLOAT3 rotationAxis, float speed, float selfspeed = XM_PIDIV2)
{
	auto verts = CreateSphere(
		size,
		16,
		16,
		color
	);

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
	Objects->insert({ name, std::make_shared<OrbitObject>(components, target, planetOffset, rotationAxis, speed, selfspeed) });
}


void Game::CreateObjects()
{
	CreateOrbitingSphere("sun", DirectX::XMFLOAT4(1, 1, 0, 1), 1.5f, nullptr, { 3,0,0 }, { 0,1,0 }, XM_PI/8);
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
	CreateOrbitingSphere("neptun", DirectX::XMFLOAT4(0.53f, 0.75f, 0.96f, 1), 0.6f, Objects->find("sun")->second, { 16,0,0 }, { 0,1,0 }, XM_PI / 7.5);

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
			mCam.mRadius = std::clamp(mCam.mRadius, 1.0f, 50.0f);
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
		DirectX::XMVectorSet(0, 0, -12, 1),
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

	CreateObjects();

	ChangeMouseModeToFPS();

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
	if (orbiting_camera)
	{
		mCam.UpdateOrbit();
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
