#pragma once
#include <SimpleMath.h>
#include "GameObject.h"
class OrbitObject : public GameObject {
public:
    std::shared_ptr<GameObject> target = nullptr;
    float selfSpeed = XM_PIDIV2;
    XMVECTOR selfAxis = XMVectorSet(0, 1, 0, 0);
    DirectX::XMVECTOR offset;        
    DirectX::XMVECTOR rotationAxis;  
    float angularSpeed;     

    OrbitObject(std::shared_ptr<GameComponent> visuals, 
        std::shared_ptr<GameObject> targetObj, DirectX::XMFLOAT3 offsetVec,
        DirectX::XMFLOAT3 axis, float speed, float selfspeed = XM_PIDIV2)
    {
        visual = visuals;
        target = targetObj;
        selfSpeed = selfspeed;
        offset = XMLoadFloat3(&offsetVec);
        rotationAxis = DirectX::XMVector3Normalize(XMLoadFloat3(&axis));
        angularSpeed = speed;
    }

    void Update(float deltaTime, float totalTime) override {
        if (!target) return;
        XMVECTOR currentRot = XMLoadFloat4(&transform.rotation);

        XMVECTOR deltaRot = XMQuaternionRotationAxis(selfAxis, selfSpeed * deltaTime);

        currentRot = XMQuaternionMultiply(currentRot, deltaRot);
        currentRot = XMQuaternionNormalize(currentRot);

        XMStoreFloat4(&transform.rotation, currentRot);
        DirectX::XMVECTOR center = XMLoadFloat3(&target->transform.position);
        float angle = angularSpeed * totalTime;
        DirectX::XMVECTOR qRotation = DirectX::XMQuaternionRotationAxis(rotationAxis, angle);

        DirectX::XMVECTOR newPos = center + DirectX::XMVector3Rotate(offset, qRotation);

        DirectX::XMStoreFloat3(&transform.position, newPos);
        XMStoreFloat3(&visual->transform.offset, newPos);
    }
};