#pragma once
#include "GameComponent.h"
#include <vector>
#include <d3d11.h>
#include <memory>
class GameObject
{
public:
	std::vector<std::shared_ptr<GameComponent>> visual;
	void move(DirectX::XMFLOAT3 direction);
	GameObject(std::vector<std::shared_ptr<GameComponent>> visuals);
	GameObject() = default;
	void Update(float deltaTime, float totalTime);
	void Draw(ID3D11DeviceContext* context);
};