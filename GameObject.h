#pragma once
#include "GameComponent.h"
#include <vector>
#include <d3d11.h>
#include <memory>
#include "PhysicsComponent.h"
class GameObject
{
public:
	XMFLOAT3 position{ 0,0,0 };
	XMFLOAT4 rotation{ 0,0,0,1 };
	std::vector<std::shared_ptr<GameComponent>> visual;
	PhysicsComponent physics{};
	BoundingBox boundingBox{};
	void move(DirectX::XMFLOAT3 direction);
	void move_teleport(DirectX::XMFLOAT3 position);
	GameObject(std::vector<std::shared_ptr<GameComponent>> visuals);
	GameObject() = default;
	virtual void Update(float deltaTime, float totalTime);
	void Draw(ID3D11DeviceContext* context);


	void UpdateBoundingBox();
	bool CheckCollision(GameObject& other);
	void ResolveCollision(GameObject& other, float deltaTime);
	void HandleWallCollision(float topBound, float bottomBound);
};