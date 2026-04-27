#pragma once
#include "DirectXDevice.h"
#include <chrono>
#include "ShaderProgram.h"
#include "GameComponent.h"
#include <vector>
#include <cmath>
#include "Keys.h"
#include <map>
#include <string>
#include "GameObject.h"
#include "Camera.h"
#include "OrbitObject.h"
#include <WICTextureLoader.h>

class InputDevice;
class DisplayWin32;

class Game
{
public:
	struct CameraBuffer
	{
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX normalMatrix;
		float time;
		XMFLOAT3 modelOffset;
	};

	struct LightBuffer
	{
		XMFLOAT3 lightDir;
		float pad1;
		XMFLOAT3 lightColor;
		float pad2;
	};

	struct CameraPS
	{
		XMFLOAT3 cameraPos;
		float pad;
	};

	struct ShadowBuffer
	{
		DirectX::XMMATRIX lightViewProj;
	};
	struct ShadowTintParams
	{
		float maxShadowDist;
		float shadowTintStrength;
		float padding[2];
	};

	ID3D11Buffer* cameraCB = nullptr;
	ID3D11Buffer* lightCB = nullptr;
	ID3D11Buffer* materialCB = nullptr;
	ID3D11Buffer* cameraPSCB = nullptr;
	ID3D11Buffer* worldCB = nullptr;
	ID3D11Buffer* shadowCB = nullptr;

	std::vector<std::shared_ptr<GameComponent>> triangles;
	DirectXDevice Device;
	std::chrono::steady_clock::time_point PrevTime;
	float TotalTime = 0;
	float TotalTimeForFPS = 0;
	unsigned int FrameCount = 0;

	DisplayWin32* Display;
	InputDevice* Input;

	virtual void Draw(float deltaTime);
	virtual void EndFrame();
	void CreateOrbitingCube(std::string name, DirectX::XMFLOAT4 color, float size, std::shared_ptr<GameObject> target, XMFLOAT3 planetOffset, XMFLOAT3 rotationAxis, float speed, float selfspeed);
	void CreateOrbitingSphere(std::string name, DirectX::XMFLOAT4 color, float size, std::shared_ptr<GameObject> target, XMFLOAT3 planetOffset, XMFLOAT3 rotationAxis, float speed, float selfspeed);
	std::shared_ptr<GameObject> GetGameObject(std::string name);
	void CreateModelObject(std::string name, std::string model_path, XMFLOAT3 collision_size, XMFLOAT3 pos);
	void EnablePhysicsCheck();
	void CreateObjects();
	void ChangeMouseModeToFPS();
	void ChangeMouseModeToOrbiting();
	void ChangeCameraModeToOrthographic();
	void ChangeCameraModeToPerspective();
	virtual void Initialize();
	virtual void PrepareFrame();
	virtual void Update(float deltaTime);
	void ClampPlayerY(GameObject& player, float topBound, float bottomBound, float speed);
	void Run();

	Game();
	std::map<std::string, std::shared_ptr<GameObject>>* Objects;
	std::map<std::string, int>* Leaderboard;
	enum GoalResult;
	GoalResult CheckGoal(GameObject& ball, float leftBound, float rightBound);
	Camera mCam{};
	struct PointLightBuffer
	{
		DirectX::XMFLOAT3 Position;
		float             Range;
		DirectX::XMFLOAT3 Color;
		float             Intensity;
		uint32_t          Enabled;
		DirectX::XMFLOAT3 Padding;
	};
	const int MAX_POINT_LIGHTS = 8;
	struct PointLightArray
	{
		PointLightBuffer lights[8];
		uint32_t numPointLights;
		DirectX::XMFLOAT3 padding;
	};
	ID3D11Buffer* pointLightCB = nullptr;
	std::shared_ptr<GameObject> currentProjectile;
private:
	// Ресурсы карты теней
	ID3D11Texture2D* shadowMapTex = nullptr;
	ID3D11DepthStencilView* shadowMapDSV = nullptr;
	ID3D11ShaderResourceView* shadowMapSRV = nullptr;
	ID3D11SamplerState* shadowSampler = nullptr;
	ID3D11Buffer* shadowWorldCB = nullptr;
	ID3D11ShaderResourceView* paletteSRV = nullptr;
	ID3D11Buffer* shadowTintCB = nullptr;
	float shadowMaxDistance = 50.0f;
	float shadowTintStrength = 0.9f;
	// Размер карты теней
	static constexpr UINT SHADOW_MAP_SIZE = 2048*4;

	ShaderProgram shadowShaderProgram;

	XMFLOAT3 lightDirection = XMFLOAT3(0.7f, -1.0f, 0.7f);
	float    lightDistance = 200.0f;   // расстояние от центра сцены до источника
	float    lightOrthoSize = 200.0f;   // размер ортогональной проекции

	void RenderShadowMap();
	ID3D11RasterizerState* rastState;
	ShaderProgram shaderProgram;
	XMMATRIX currentLightViewProj;  // Сохраняем матрицу для использования в Draw
};