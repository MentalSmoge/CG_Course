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
class InputDevice;
class DisplayWin32;
class Game
{
public:
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
	virtual void Initialize();
	virtual void PrepareFrame();
	//virtual void PrepareResources();
	virtual void Update(float deltaTime);
	//virtual void UpdateInternal();
	//void Exit();
	//void MessageHandler();
	//void RestoreTargets();
	void Run();
	Game();
	std::map<std::string, GameObject>* Objects;
private:
	//void CreateBackBuffer();
	ID3D11RasterizerState* rastState;
	ShaderProgram shaderProgram;
};