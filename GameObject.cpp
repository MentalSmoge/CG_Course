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
GameObject::GameObject(std::shared_ptr<GameComponent> visuals, XMFLOAT3 size, XMFLOAT3 pos)
{
    transform.position.x = pos.x;
    transform.position.y = pos.y;
    transform.position.z = pos.z;
	visual = visuals;
    physics.size = size;
    UpdateBoundingBox();
    mb.ambient = { 0.3f, 0.3f, 0.3f };
    mb.diffuse = { 1.0f, 1.0f, 1.0f };
    mb.specular = { 1.0f, 1.0f, 1.0f };
    mb.shininess = 32.0f;
}

void GameObject::Update(float deltaTime, float totalTime)
{
    if (isProjectile)
    {
        transform.position.x += velocity.x * deltaTime;
        transform.position.y += velocity.y * deltaTime;
        transform.position.z += velocity.z * deltaTime;

        velocity.y += gravity * deltaTime;

        if (transform.position.y <= -1.0f)
        {
            transform.position.y = -1.0f;
            velocity = { 0,0,0 };
            isProjectile = false;
            //hasPointLight = false;
        }

        UpdateBoundingBox();
    }
    else
    {
        if (parent != nullptr)
        {
            DirectX::XMVECTOR parentPos = DirectX::XMLoadFloat3(&parent->transform.position);
            DirectX::XMVECTOR parentRot = DirectX::XMLoadFloat4(&parent->transform.rotation);
            DirectX::XMVECTOR localPos = DirectX::XMLoadFloat3(&localPosition);
            DirectX::XMVECTOR worldPos = DirectX::XMVectorAdd(parentPos, DirectX::XMVector3Rotate(localPos, parentRot));
            DirectX::XMStoreFloat3(&transform.position, worldPos);

            DirectX::XMVECTOR localRot = DirectX::XMLoadFloat4(&localRotation);
            DirectX::XMVECTOR worldRot = DirectX::XMQuaternionMultiply(localRot, parentRot);
            DirectX::XMStoreFloat4(&transform.rotation, worldRot);
        }
        UpdateBoundingBox();
    }

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

    if (parent)
    {
        auto it = std::find(parent->attachedObjects.begin(), parent->attachedObjects.end(), shared_from_this());
        if (it != parent->attachedObjects.end())
            parent->attachedObjects.erase(it);
    }

    parent = newParent;

    parent->attachedObjects.push_back(shared_from_this());

    DirectX::XMVECTOR worldPos = DirectX::XMLoadFloat3(&transform.position);
    DirectX::XMVECTOR parentPos = DirectX::XMLoadFloat3(&parent->transform.position);
    DirectX::XMVECTOR parentRot = DirectX::XMLoadFloat4(&parent->transform.rotation);
    DirectX::XMVECTOR offset = DirectX::XMVectorSubtract(worldPos, parentPos);
    DirectX::XMVECTOR localPos = DirectX::XMVector3Rotate(offset, DirectX::XMQuaternionInverse(parentRot));
    DirectX::XMStoreFloat3(&localPosition, localPos);

    DirectX::XMVECTOR worldRot = DirectX::XMLoadFloat4(&transform.rotation);
    DirectX::XMVECTOR localRot = DirectX::XMQuaternionMultiply(worldRot, DirectX::XMQuaternionInverse(parentRot));
    DirectX::XMStoreFloat4(&localRotation, localRot);
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

void GameObject::Shoot(DirectX::XMFLOAT3 direction, float speed)
{
    if (parent)
    {
        auto it = std::find(parent->attachedObjects.begin(), parent->attachedObjects.end(), shared_from_this());
        if (it != parent->attachedObjects.end())
            parent->attachedObjects.erase(it);
        parent = nullptr;
    }

    DirectX::XMVECTOR dirVec = DirectX::XMLoadFloat3(&direction);
    dirVec = DirectX::XMVector3Normalize(dirVec);
    DirectX::XMStoreFloat3(&velocity, dirVec * speed);
    if (transform.position.y < -0.99)
        transform.position.y = -0.99;
    isProjectile = true;
}
