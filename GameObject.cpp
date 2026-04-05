#include "GameObject.h"
#include <iostream>

void GameObject::move(DirectX::XMFLOAT3 direction) {
    visual->transform.position.x += direction.x;
    visual->transform.position.y += direction.y;
    visual->transform.position.z += direction.z;
    position.x += direction.x;
    position.y += direction.y;
    position.z += direction.z;
}

void GameObject::move_teleport(DirectX::XMFLOAT3 new_position)
{
    visual->transform.position.x = new_position.x;
    visual->transform.position.y = new_position.y;
    visual->transform.position.z = new_position.z;
    position.x = new_position.x;
    position.y = new_position.y;
    position.z = new_position.z;
}

GameObject::GameObject(std::shared_ptr<GameComponent> visuals)
{
	visual = visuals;
}

void GameObject::Update(float deltaTime, float totalTime)
{
    move({ physics.velocity.x * deltaTime, physics.velocity.y * deltaTime, 0 });
    UpdateBoundingBox();
        visual->Update(deltaTime, totalTime);
    
}

void GameObject::Draw(ID3D11DeviceContext* context)
{
        visual->Draw(context);
    
}

void GameObject::UpdateBoundingBox()
{
    XMFLOAT3 center = position;

    boundingBox.Center = center;
    boundingBox.Extents = XMFLOAT3(physics.size.x, physics.size.y, 0);
}

bool GameObject::CheckCollision(GameObject& other)
{
    return boundingBox.Intersects(other.boundingBox);
}

void GameObject::ResolveCollision(GameObject& other, float deltaTime)
{
    float ballY = this->position.y;
    float paddleY = other.position.y;

    float paddleHalfHeight = other.physics.size.y;

    float relativeIntersectY = ballY - paddleY;

    float normalized = relativeIntersectY / paddleHalfHeight;

    float maxBounceAngle = DirectX::XMConvertToRadians(30.0f);

    float bounceAngle = normalized * maxBounceAngle;

    float speed = sqrt(
        physics.velocity.x * physics.velocity.x +
        physics.velocity.y * physics.velocity.y
    );
    speed = speed * 1.2f;

    float direction = (physics.velocity.x > 0) ? -1.0f : 1.0f;

    physics.velocity.x = direction * speed * cos(bounceAngle);
    physics.velocity.y = speed * sin(bounceAngle);
}

void GameObject::HandleWallCollision(float topBound, float bottomBound)
{
    float ballY = position.y;
    float halfHeight = physics.size.y;

    if (ballY + halfHeight >= topBound)
    {
        position.y = topBound - halfHeight;

        physics.velocity.y = -physics.velocity.y;
    }

    if (ballY - halfHeight <= bottomBound)
    {
        position.y = bottomBound + halfHeight;

        physics.velocity.y = -physics.velocity.y;
    }

    UpdateBoundingBox();
}