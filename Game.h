#pragma once
#include "DisplayWin32.h"
#include "DirectXDevice.h"
#include <chrono>
#include "ShaderProgram.h"
#include "GameComponent.h"
#include <vector>
#include <cmath>
class Game
{
public:
	std::vector<GameComponent> triangles;
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

	DisplayWin32 Display;
	//InputDevice InputDevice;
	virtual void DestroyResources();
	virtual void Draw();
	virtual void EndFrame();
	virtual void Initialize();
	virtual void PrepareFrame();
	virtual void PrepareResources();
	virtual void Update();
	virtual void UpdateInternal();
	void Exit();
	void MessageHandler();
	void RestoreTargets();
	void Run();
	Game();
private:
	void CreateBackBuffer();
	ID3D11RasterizerState* rastState;
	ShaderProgram shaderProgram;
};