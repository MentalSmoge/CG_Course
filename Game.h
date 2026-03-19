#pragma once
#include "DisplayWin32.h"
#include "InputDevice.h"
class Game
{
public:
	//backBuffer
	//Context
	//DebugAnnotation
	//Device
	//Instance
	//Name
	//PrevTime
	//RenderSRV
	//RenderView
	//ScreenResized
	//StartTime
	//SwapChain
	//TotalTime
	DisplayWin32 Display;
	InputDevice InputDevice;
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
};