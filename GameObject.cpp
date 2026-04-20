#include "GameObject.h"
#include <iostream>

void GameObject::move(DirectX::XMFLOAT3 direction) {
    transform.position.x += direction.x;
    transform.position.y += direction.y;
    transform.position.z += direction.z;
}

void GameObject::move_teleport(DirectX::XMFLOAT3 new_position)
{
    transform.position.x = new_position.x;
    transform.position.y = new_position.y;
    transform.position.z = new_position.z;
}

GameObject::GameObject(std::shared_ptr<GameComponent> visuals)
{
	visual = visuals;

    mb.ambient = { 0.3f, 0.3f, 0.3f };
    mb.diffuse = { 1.0f, 1.0f, 1.0f };
    mb.specular = { 1.0f, 1.0f, 1.0f };
    mb.shininess = 32.0f;
}
GameObject::GameObject(std::shared_ptr<GameComponent> visuals, XMFLOAT3 size)
{
	visual = visuals;
    physics.size = size;

    mb.ambient = { 0.3f, 0.3f, 0.3f };
    mb.diffuse = { 1.0f, 1.0f, 1.0f };
    mb.specular = { 1.0f, 1.0f, 1.0f };
    mb.shininess = 32.0f;
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

void GameObject::AttachToParent(std::shared_ptr<GameObject> newParent)
{
    if (!newParent || parent == newParent) return;

    parent = newParent;

    XMVECTOR worldPos = XMLoadFloat3(&transform.position);
    XMVECTOR parentPos = XMLoadFloat3(&parent->transform.position);
    XMVECTOR parentRot = XMLoadFloat4(&parent->transform.rotation);

    XMVECTOR offset = XMVectorSubtract(worldPos, parentPos);

    XMVECTOR localPos = XMVector3Rotate(offset, XMQuaternionInverse(parentRot));
    XMStoreFloat3(&localPosition, localPos);

    XMVECTOR worldRot = XMLoadFloat4(&transform.rotation);
    XMVECTOR localRot = XMQuaternionMultiply(worldRot, XMQuaternionInverse(parentRot));
    XMStoreFloat4(&localRotation, localRot);
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