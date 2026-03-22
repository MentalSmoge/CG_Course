#include "GameObject.h"

void GameObject::move(DirectX::XMFLOAT3 direction) {
    for (auto& component : visual) {
        component->transform.position.x += direction.x;
        component->transform.position.y += direction.y;
        component->transform.position.z += direction.z;
    }
}

GameObject::GameObject(std::vector<std::shared_ptr<GameComponent>> visuals)
{
	visual = visuals;
}

void GameObject::Update(float deltaTime, float totalTime)
{
    for (auto& component : visual)
    {
        component->Update(deltaTime, totalTime);
    }
}

void GameObject::Draw(ID3D11DeviceContext* context)
{
    for (auto& component : visual)
    {
        component->Draw(context);
    }
}
