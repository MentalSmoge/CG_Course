#include "GameObject.h"
#include <iostream>

void GameObject::move(DirectX::XMFLOAT3 direction) {
    /*visual->transform.position.x += direction.x;
    visual->transform.position.y += direction.y;
    visual->transform.position.z += direction.z;*/
    transform.position.x += direction.x;
    transform.position.y += direction.y;
    transform.position.z += direction.z;
}

void GameObject::move_teleport(DirectX::XMFLOAT3 new_position)
{
    /*visual->transform.position.x = new_position.x;
    visual->transform.position.y = new_position.y;
    visual->transform.position.z = new_position.z;*/
    transform.position.x = new_position.x;
    transform.position.y = new_position.y;
    transform.position.z = new_position.z;
}

GameObject::GameObject(std::shared_ptr<GameComponent> visuals)
{
	visual = visuals;
}
GameObject::GameObject(std::shared_ptr<GameComponent> visuals, XMFLOAT3 size)
{
	visual = visuals;
    physics.size = size;
}

void GameObject::Update(float deltaTime, float totalTime)
{
    if (parent != nullptr)
    {
        XMVECTOR parentPos = XMLoadFloat3(&parent->transform.position);
        XMVECTOR parentRot = XMLoadFloat4(&parent->transform.rotation);

        XMVECTOR localPos = XMLoadFloat3(&localPosition);

        XMVECTOR worldPos = XMVectorAdd(parentPos, XMVector3Rotate(localPos, parentRot));
        XMStoreFloat3(&transform.position, worldPos);

        XMVECTOR localRot = XMLoadFloat4(&localRotation);
        XMVECTOR worldRot = XMQuaternionMultiply(localRot, parentRot);
        XMStoreFloat4(&transform.rotation, worldRot);

    }
    UpdateBoundingBox();
    visual->Update(deltaTime, totalTime);
    
}

void GameObject::Draw(ID3D11DeviceContext* context)
{
    visual->Draw(context);
    UpdateBoundingBoxVertices(context);
    DrawBoundingBox(context);
}

void GameObject::UpdateBoundingBox()
{
    XMFLOAT3 center = transform.position;

    boundingBox.Center = center;
    boundingBox.Extents = {
        physics.size.x * transform.scale.x,
        physics.size.y * transform.scale.y,
        physics.size.z * transform.scale.z
    };
}

bool GameObject::CheckCollision(std::shared_ptr<GameObject> other)
{
    return boundingBox.Intersects(other->boundingBox);
}

void GameObject::ResolveCollision(GameObject& other, float deltaTime)
{

}

void GameObject::Rotate(XMFLOAT3 axis, float angle)
{
    XMVECTOR q = XMLoadFloat4(&transform.rotation);
    XMVECTOR dq = XMQuaternionRotationAxis(XMLoadFloat3(&axis), angle);

    q = XMQuaternionMultiply(q, dq);
    XMStoreFloat4(&transform.rotation, q);
}
void GameObject::SetScale(float s)
{
    transform.scale = { s, s, s };
}