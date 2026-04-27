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
bool start_check = false;
float input_cooldown = 0.0f;
float grace_period = 0.0f;
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
void Game::RenderShadowMap()
{
	D3D11_VIEWPORT prevVP;
	UINT numVP = 1;
	Device.context->RSGetViewports(&numVP, &prevVP);

	ID3D11RenderTargetView* prevRTV = nullptr;
	ID3D11DepthStencilView* prevDSV = nullptr;
	Device.context->OMGetRenderTargets(1, &prevRTV, &prevDSV);

	Device.context->OMSetRenderTargets(0, nullptr, shadowMapDSV);
	Device.context->ClearDepthStencilView(shadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	D3D11_VIEWPORT shadowVP = {};
	shadowVP.Width = static_cast<float>(SHADOW_MAP_SIZE);
	shadowVP.Height = static_cast<float>(SHADOW_MAP_SIZE);
	shadowVP.MinDepth = 0.0f;
	shadowVP.MaxDepth = 1.0f;
	shadowVP.TopLeftX = 0;
	shadowVP.TopLeftY = 0;
	Device.context->RSSetViewports(1, &shadowVP);

	XMVECTOR lightDirVec = XMLoadFloat3(&lightDirection);
	lightDirVec = XMVector3Normalize(lightDirVec);

	XMVECTOR sceneCenter = XMVectorSet(lightOrthoSize/2, 0.0f, 0.0f, 1.0f);

	XMVECTOR lightPos = sceneCenter - lightDirVec * lightDistance;

	XMMATRIX lightView = XMMatrixLookAtLH(
		lightPos,
		sceneCenter,
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
	);

	XMMATRIX lightProj = XMMatrixOrthographicLH(
		lightOrthoSize,
		lightOrthoSize,
		0.1f,
		lightDistance
	);

	XMMATRIX lightViewProj = lightView * lightProj;
	currentLightViewProj = lightViewProj;

	ShadowBuffer sb; sb.lightViewProj = XMMatrixTranspose(currentLightViewProj);
	Device.context->UpdateSubresource(shadowCB, 0, nullptr, &sb, 0, 0);
	Device.context->VSSetConstantBuffers(1, 1, &shadowCB);

	Device.context->VSSetShader(shadowShaderProgram.vertexShader, nullptr, 0);
	Device.context->PSSetShader(nullptr, nullptr, 0);
	Device.context->IASetInputLayout(shadowShaderProgram.inputLayout);

	for (auto& [key, obj] : *Objects)
	{
		XMMATRIX world = XMMatrixScaling(
			obj->transform.scale.x,
			obj->transform.scale.y,
			obj->transform.scale.z
		) *
			XMMatrixRotationQuaternion(XMLoadFloat4(&obj->transform.rotation)) *
			XMMatrixTranslation(
				obj->transform.position.x,
				obj->transform.position.y,
				obj->transform.position.z
			);

		XMMATRIX worldTransposed = XMMatrixTranspose(world);
		Device.context->UpdateSubresource(shadowWorldCB, 0, nullptr, &worldTransposed, 0, 0);
		Device.context->VSSetConstantBuffers(0, 1, &shadowWorldCB);

		obj->Draw(Device.context);
	}

	Device.context->OMSetRenderTargets(1, &prevRTV, prevDSV);
	if (prevRTV) prevRTV->Release();
	if (prevDSV) prevDSV->Release();

	Device.context->RSSetViewports(1, &prevVP);
	Device.context->VSSetShader(shaderProgram.vertexShader, nullptr, 0);
	Device.context->PSSetShader(shaderProgram.pixelShader, nullptr, 0);
	Device.context->IASetInputLayout(shaderProgram.inputLayout);
}
void Game::Draw(float deltaTime)
{
	RenderShadowMap();

	Device.context->PSSetShaderResources(2, 1, &paletteSRV);
	ShadowTintParams stParams;
	stParams.maxShadowDist = shadowMaxDistance;
	stParams.shadowTintStrength = shadowTintStrength;
	Device.context->UpdateSubresource(shadowTintCB, 0, nullptr, &stParams, 0, 0);
	Device.context->PSSetConstantBuffers(7, 1, &shadowTintCB);

	mCam.UpdateViewMatrix();

	CameraBuffer cb{};
	cb.view = DirectX::XMMatrixTranspose(mCam.View());
	cb.proj = DirectX::XMMatrixTranspose(mCam.Proj());
	cb.time = TotalTime;
	Device.context->UpdateSubresource(cameraCB, 0, nullptr, &cb, 0, 0);
	Device.context->VSSetConstantBuffers(0, 1, &cameraCB);

	Device.context->OMSetRenderTargets(1, &Device.rtv, depthStencilView);

	float clearColor[] = { 0.1f, 0.1f, 0.15f, 1.0f };
	Device.context->ClearRenderTargetView(Device.rtv, clearColor);
	Device.context->ClearDepthStencilView(
		depthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0
	);

	Device.context->PSSetShaderResources(1, 1, &shadowMapSRV);
	Device.context->PSSetSamplers(1, 1, &shadowSampler);

	XMMATRIX lightViewProj = currentLightViewProj;

	ShadowBuffer sb;
	sb.lightViewProj = XMMatrixTranspose(lightViewProj);
	Device.context->UpdateSubresource(shadowCB, 0, nullptr, &sb, 0, 0);
	Device.context->VSSetConstantBuffers(5, 1, &shadowCB);
	Device.context->PSSetConstantBuffers(5, 1, &shadowCB);
	PointLightArray plArray = {};
	plArray.numPointLights = 0;

	for (auto& [key, obj] : *Objects)
	{
		if (obj->hasPointLight && plArray.numPointLights < MAX_POINT_LIGHTS)
		{
			auto& light = plArray.lights[plArray.numPointLights];
			light.Position = obj->transform.position;
			light.Range = obj->pointLightRange;
			light.Color = obj->pointLightColor;
			light.Intensity = obj->pointLightIntensity;
			light.Enabled = 1;

			plArray.numPointLights++;
		}
	}

	Device.context->UpdateSubresource(pointLightCB, 0, nullptr, &plArray, 0, 0);
	Device.context->PSSetConstantBuffers(6, 1, &pointLightCB);

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
		XMMATRIX normalMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, world));

		cb.normalMatrix = XMMatrixTranspose(normalMatrix);
		cb.modelOffset = value->visual->transform.offset;
		Device.context->UpdateSubresource(cameraCB, 0, nullptr, &cb, 0, 0);

		LightBuffer lb;
		lb.lightDir = lightDirection;
		lb.lightColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
		Device.context->UpdateSubresource(lightCB, 0, nullptr, &lb, 0, 0);
		Device.context->PSSetConstantBuffers(2, 1, &lightCB);

		Device.context->UpdateSubresource(materialCB, 0, nullptr, &value->mb, 0, 0);
		Device.context->PSSetConstantBuffers(3, 1, &materialCB);

		CameraPS cam;
		cam.cameraPos = mCam.GetPosition();
		Device.context->UpdateSubresource(cameraPSCB, 0, nullptr, &cam, 0, 0);
		Device.context->PSSetConstantBuffers(4, 1, &cameraPSCB);

		value->Draw(Device.context);
	}

	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	Device.context->PSSetShaderResources(1, 1, nullSRV);
	Device.context->PSSetShaderResources(2, 1, nullSRV);

	ID3D11SamplerState* nullSampler[1] = { nullptr };
	Device.context->PSSetSamplers(1, 1, nullSampler);


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

	mesh.vertices.push_back({ { -hw, -hh,  hd, 1 }, { 0, 0, 1 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw, -hh,  hd, 1 }, { 0, 0, 1 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw,  hh,  hd, 1 }, { 0, 0, 1 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw,  hh,  hd, 1 }, { 0, 0, 1 }, {1,1}, color });

	mesh.vertices.push_back({ {  hw, -hh, -hd, 1 }, { 0, 0, -1 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw, -hh, -hd, 1 }, { 0, 0, -1 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw,  hh, -hd, 1 }, { 0, 0, -1 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw,  hh, -hd, 1 }, { 0, 0, -1 }, {1,1}, color });

	mesh.vertices.push_back({ { -hw,  hh,  hd, 1 }, { 0, 1, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw,  hh,  hd, 1 }, { 0, 1, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw,  hh, -hd, 1 }, { 0, 1, 0 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw,  hh, -hd, 1 }, { 0, 1, 0 }, {1,1}, color });

	mesh.vertices.push_back({ { -hw, -hh, -hd, 1 }, { 0, -1, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw, -hh, -hd, 1 }, { 0, -1, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw, -hh,  hd, 1 }, { 0, -1, 0 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw, -hh,  hd, 1 }, { 0, -1, 0 }, {1,1}, color });

	mesh.vertices.push_back({ { -hw, -hh, -hd, 1 }, { -1, 0, 0 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw, -hh,  hd, 1 }, { -1, 0, 0 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw,  hh,  hd, 1 }, { -1, 0, 0 }, {1,1}, color });
	mesh.vertices.push_back({ { -hw,  hh, -hd, 1 }, { -1, 0, 0 }, {1,1}, color });

	mesh.vertices.push_back({ {  hw, -hh,  hd, 1 }, { 1, 0, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw, -hh, -hd, 1 }, { 1, 0, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw,  hh, -hd, 1 }, { 1, 0, 0 }, {1,1}, color });
	mesh.vertices.push_back({ {  hw,  hh,  hd, 1 }, { 1, 0, 0 }, {1,1}, color });

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

	for (int i = 0; i <= stackCount; ++i)
	{
		float phi = XM_PI * i / stackCount;

		for (int j = 0; j <= sliceCount; ++j)
		{
			float theta = 2.0f * XM_PI * j / sliceCount;

			float x = radius * sinf(phi) * cosf(theta);
			float y = radius * cosf(phi);
			float z = radius * sinf(phi) * sinf(theta);

			XMFLOAT4 pos = { x, y, z, 1 };

			XMVECTOR n = XMVector3Normalize(XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(&pos)));
			XMFLOAT3 normal;
			XMStoreFloat3(&normal, n);

			mesh.vertices.push_back({ pos, normal, {1,1}, color });
		}
	}

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

void Game::CreateModelObject(std::string name, std::string model_path, XMFLOAT3 collision_size, XMFLOAT3 pos)
{
	auto mesh = ModelLoader().LoadModel(Device.device, Device.context, model_path);

	auto component = std::make_shared<GameComponent>(
		Device.device,
		mesh.vertices,
		mesh.indices,
		mesh.texture
	);
	auto obj = std::make_shared<GameObject>(component, collision_size, pos);
	obj->InitBoundingBoxBuffers(Device.device);
	Objects->insert({ name, obj });
}
void Game::EnablePhysicsCheck()
{
	start_check = true;

}
void Game::CreateObjects()
{
	CreateModelObject("ball", "Models/beach_ball.glb", XMFLOAT3{ 10.5f,10,10.1f }, {0,0,0});
	GetGameObject("ball")->SetScale(0.1f);

	CreateModelObject("ball2", "Models/beach_ball.glb", XMFLOAT3{ 10.5f,10,10.1f }, { 7.15f, 0, -2 });
	GetGameObject("ball2")->SetScale(0.1f);
	ObjectsToCheckForCollision.push_back(GetGameObject("ball2"));
	GetGameObject("ball2")->mb.shininess = 89.6f;
	GetGameObject("ball2")->mb.specular = { 0.773911f, 0.773911f, 0.773911f };
	GetGameObject("ball2")->mb.diffuse = { 0.2775f, 0.2775f, 0.2775f };
	GetGameObject("ball2")->mb.ambient = { 0.23125f, 0.23125f, 0.23125f };
	GetGameObject("ball2")->pointLightColor = { 0.2f, 0.6f, 1.0f };


	CreateModelObject("ball3", "Models/beach_ball.glb", XMFLOAT3{ 10.5f,10,10.1f }, { -40.15f, 25, -40 });

	//CreateModelObject("toilet", "Models/shrek_toilet.glb", XMFLOAT3 {0.5f,1,0.1f}, { 5.15f, 2, 0 });
	//GetGameObject("toilet")->visual->transform.offset = XMFLOAT3{ -0, -1.5, -1.0 };
	////GetGameObject("toilet")->move({ 5.15f, 2, 0 });
	//GetGameObject("toilet")->SetScale(2.15f);
	//ObjectsToCheckForCollision.push_back(GetGameObject("toilet"));

	CreateModelObject("table", "Models/end_table.glb", XMFLOAT3{ 10.5f,10,10.1f }, { -5.15f, 0, 0 });
	GetGameObject("table")->SetScale(0.04f);
	ObjectsToCheckForCollision.push_back(GetGameObject("table"));
	GetGameObject("table")->mb.shininess = 27.8974;
	GetGameObject("table")->mb.specular = { 0.992157f, 0.941176f, 0.807843f };
	GetGameObject("table")->mb.diffuse = { 0.780392f, 0.568627f, 0.113725f };
	GetGameObject("table")->mb.ambient = { 0.329412f, 0.223529f, 0.027451f };
	GetGameObject("table")->pointLightColor = { 0.0f, 1.0f, 0.0f };

	CreateModelObject("table2", "Models/end_table.glb", XMFLOAT3{ 10.5f,10,10.1f }, { -5.15f, 0, 4 });
	GetGameObject("table2")->SetScale(0.04f);
	ObjectsToCheckForCollision.push_back(GetGameObject("table2"));

	CreateModelObject("dino", "Models/allosaurus_carnivores.glb", XMFLOAT3{ 10.5f,10,10.1f }, { -0, 0, 5 });
	GetGameObject("dino")->SetScale(0.02f);
	ObjectsToCheckForCollision.push_back(GetGameObject("dino"));
	GetGameObject("dino")->mb.shininess = 32;
	GetGameObject("dino")->mb.specular = { 0.5f, 0.5f, 0.5f };
	GetGameObject("dino")->mb.diffuse = { 0.01f, 0.01f, 0.01f };
	GetGameObject("dino")->mb.ambient = { 0.0f, 0.0f, 0.0f };
	GetGameObject("dino")->pointLightColor = { 1.0f, 0.0f, 0.0f }; 

	CreateModelObject("dino2", "Models/allosaurus_carnivores.glb", XMFLOAT3{ 10.5f,10,10.1f }, { -3, 0, 5 });
	GetGameObject("dino2")->SetScale(0.02f);
	GetGameObject("dino2")->pointLightColor = { 0.8f, 0.0f, 1.0f };
	ObjectsToCheckForCollision.push_back(GetGameObject("dino2"));

	CreateModelObject("floor", "Models/checkered_tile_floor.glb", XMFLOAT3{ 0.5f,0.1f,0.1f }, { -0, -1.5, 0 });
	GetGameObject("floor")->Rotate(XMFLOAT3(1.0f, 0.0f, 0.0f), -XM_PIDIV2);
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


	D3D11_BUFFER_DESC lbd = {};
	lbd.Usage = D3D11_USAGE_DEFAULT;
	lbd.ByteWidth = sizeof(LightBuffer);
	lbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	Device.device->CreateBuffer(&lbd, nullptr, &lightCB);

	D3D11_BUFFER_DESC mbd = {};
	mbd.Usage = D3D11_USAGE_DEFAULT;
	mbd.ByteWidth = sizeof(GameObject::MaterialBuffer);
	mbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	Device.device->CreateBuffer(&mbd, nullptr, &materialCB);

	D3D11_BUFFER_DESC cpsbd = {};
	cpsbd.Usage = D3D11_USAGE_DEFAULT;
	cpsbd.ByteWidth = sizeof(CameraPS);
	cpsbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	Device.device->CreateBuffer(&cpsbd, nullptr, &cameraPSCB);

	D3D11_BUFFER_DESC shadowBD = {};
	shadowBD.Usage = D3D11_USAGE_DEFAULT;
	shadowBD.ByteWidth = sizeof(ShadowBuffer);
	shadowBD.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Device.device->CreateBuffer(&shadowBD, nullptr, &shadowCB);

	D3D11_TEXTURE2D_DESC shadowTexDesc = {};
	shadowTexDesc.Width = SHADOW_MAP_SIZE;
	shadowTexDesc.Height = SHADOW_MAP_SIZE;
	shadowTexDesc.MipLevels = 1;
	shadowTexDesc.ArraySize = 1;
	shadowTexDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowTexDesc.SampleDesc.Count = 1;
	shadowTexDesc.SampleDesc.Quality = 0;
	shadowTexDesc.Usage = D3D11_USAGE_DEFAULT;
	shadowTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	Device.device->CreateTexture2D(&shadowTexDesc, nullptr, &shadowMapTex);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	Device.device->CreateDepthStencilView(shadowMapTex, &dsvDesc, &shadowMapDSV);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	Device.device->CreateShaderResourceView(shadowMapTex, &srvDesc, &shadowMapSRV);

	D3D11_BUFFER_DESC worldBD = {};
	worldBD.Usage = D3D11_USAGE_DEFAULT;
	worldBD.ByteWidth = sizeof(DirectX::XMMATRIX);
	worldBD.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Device.device->CreateBuffer(&worldBD, nullptr, &shadowWorldCB);

	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f;
	shadowSampDesc.BorderColor[1] = 1.0f;
	shadowSampDesc.BorderColor[2] = 1.0f;
	shadowSampDesc.BorderColor[3] = 1.0f;
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	Device.device->CreateSamplerState(&shadowSampDesc, &shadowSampler);

	D3D11_INPUT_ELEMENT_DESC shadowInputElem[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	shadowShaderProgram = ShaderProgram(
		Device.device,
		L"./Shaders/ShadowMap.hlsl",
		shadowInputElem,
		1,
		nullptr
	);


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
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	Device.device->CreateSamplerState(&sampDesc, &sampler);
	Device.context->PSSetSamplers(0, 1, &sampler);

	D3D11_BUFFER_DESC plDesc = {};
	plDesc.Usage = D3D11_USAGE_DEFAULT;
	plDesc.ByteWidth = sizeof(PointLightArray);
	plDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Device.device->CreateBuffer(&plDesc, nullptr, &pointLightCB);



	HRESULT hr = DirectX::CreateWICTextureFromFile(
		Device.device,
		Device.context,  // можно передать nullptr, если контекст не нужен
		L"palette.png",
		nullptr,  // не нужен объект текстуры
		&paletteSRV
	);

	D3D11_BUFFER_DESC stDesc = {};
	stDesc.Usage = D3D11_USAGE_DEFAULT;
	stDesc.ByteWidth = sizeof(ShadowTintParams);
	stDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Device.device->CreateBuffer(&stDesc, nullptr, &shadowTintCB);

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
	grace_period += deltaTime;
	if (orbiting_camera)
	{
		mCam.mTarget = GetGameObject("ball")->transform.position;
		mCam.UpdateOrbit();

		const float speed = 5.0f;

		XMVECTOR forward = -mCam.GetLookXM();
		XMVECTOR right = -mCam.GetRightXM();

		forward = XMVectorSetY(forward, 0.0f);
		right = XMVectorSetY(right, 0.0f);

		forward = XMVector3Normalize(forward);
		right = XMVector3Normalize(right);

		XMVECTOR move = XMVectorZero();

		if (Input->IsKeyDown(Keys::W)) move += forward;
		if (Input->IsKeyDown(Keys::S)) move -= forward;
		if (Input->IsKeyDown(Keys::D)) move += right;
		if (Input->IsKeyDown(Keys::A)) move -= right;

		if (!XMVector3Equal(move, XMVectorZero()))
		{
			move = XMVector3Normalize(move);
			start_check = true;
		}

		move *= -speed * deltaTime;

		XMFLOAT3 velocity;
		XMStoreFloat3(&velocity, move);

		GetGameObject("ball")->move(velocity);

		XMFLOAT3 axis{ velocity.z, 0.0f, -velocity.x };
		float length = sqrtf(axis.x * axis.x + axis.z * axis.z);

		if (length > 0.0f)
		{
			axis.x /= length;
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
	if (grace_period > 2.0f and start_check)
	{
		for (size_t i = 0; i < ObjectsToCheckForCollision.size(); )
		{
			if (GetGameObject("ball")->CheckCollision(ObjectsToCheckForCollision[i]))
			{
				ObjectsToCheckForCollision[i]->AttachToParent(GetGameObject("ball"));
				ObjectsToCheckForCollision.erase(ObjectsToCheckForCollision.begin() + i);
			}
			else
			{
				i++;
			}
		}
	}


	static float shootCooldown = 0.0f;
	if (shootCooldown > 0.0f)
		shootCooldown -= deltaTime;

	if (Input->IsKeyDown(Keys::F) && shootCooldown <= 0.0f)
	{
		auto playerBall = GetGameObject("ball");
		if (playerBall && !playerBall->attachedObjects.empty())
		{
			auto lastAttached = playerBall->attachedObjects.back();

			DirectX::XMFLOAT3 camDir = mCam.GetLook();
			DirectX::XMVECTOR dirVec = DirectX::XMLoadFloat3(&camDir);
			dirVec = DirectX::XMVectorSetY(dirVec, DirectX::XMVectorGetY(dirVec) + 0.5f);
			dirVec = DirectX::XMVector3Normalize(dirVec);

			DirectX::XMFLOAT3 shootDir;
			DirectX::XMStoreFloat3(&shootDir, dirVec);

			lastAttached->Shoot(shootDir, 15.0f);
			lastAttached->EnablePointLight(true);

			shootCooldown = 0.3f;
		}
	}
	for (auto& [key, obj] : *Objects)
	{
		if (key != "ball" && !obj->parent && !obj->isProjectile)
		{
			auto it = std::find(ObjectsToCheckForCollision.begin(), ObjectsToCheckForCollision.end(), obj);
			if (it == ObjectsToCheckForCollision.end())
				ObjectsToCheckForCollision.push_back(obj);
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
