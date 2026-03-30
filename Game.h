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
	};
	ID3D11Buffer* cameraCB = nullptr;
	ID3D11Buffer* worldCB = nullptr;
	std::vector<std::shared_ptr<GameComponent>> triangles;
	//backBuffer
	
	//Device
	//Context
	//SwapChain
	//RenderView
	DirectXDevice Device;

	//DebugAnnotation
	//Instance
	//Name
	//RenderSRV
	//ScreenResized
	
	//StartTime
	//PrevTime
	std::chrono::steady_clock::time_point PrevTime;
	//TotalTime
	float TotalTime = 0;
	float TotalTimeForFPS = 0;
	unsigned int FrameCount = 0;

	DisplayWin32* Display;
	InputDevice* Input;
	//virtual void DestroyResources();
	virtual void Draw(float deltaTime);
	virtual void EndFrame();
	void CreateOrbitingCube(std::string name, DirectX::XMFLOAT4 color, float size, std::shared_ptr<GameObject> target, XMFLOAT3 planetOffset, XMFLOAT3 rotationAxis, float speed);
	void CreateOrbitingSphere(std::string name, DirectX::XMFLOAT4 color, float size, std::shared_ptr<GameObject> target, XMFLOAT3 planetOffset, XMFLOAT3 rotationAxis, float speed);
	void CreateObjects();
	virtual void Initialize();
	virtual void PrepareFrame();
	//virtual void PrepareResources();
	virtual void Update(float deltaTime);
	void ClampPlayerY(GameObject& player, float topBound, float bottomBound, float speed);
	//virtual void UpdateInternal();
	//void Exit();
	//void MessageHandler();
	//void RestoreTargets();
	void Run();
	Game();
	std::map<std::string, std::shared_ptr<GameObject>>* Objects;
	std::map<std::string, int>* Leaderboard;
	enum GoalResult;
	GoalResult CheckGoal(GameObject& ball, float leftBound, float rightBound);
	Camera mCam{};
private:
	//void CreateBackBuffer();
	ID3D11RasterizerState* rastState;
	ShaderProgram shaderProgram;
};