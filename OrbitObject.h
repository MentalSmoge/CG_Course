#pragma once
#include <SimpleMath.h>
#include "GameObject.h"
class OrbitObject : public GameObject {
public:
    std::shared_ptr<GameObject> target = nullptr;
    float selfSpeed = XM_PIDIV2; // радиан/сек
    XMVECTOR selfAxis = XMVectorSet(0, 1, 0, 0);
    //DirectX::XMVECTOR center;        // Вокруг чего вращаем
    DirectX::XMVECTOR offset;        // Вектор от центра до объекта (радиус)
    DirectX::XMVECTOR rotationAxis;  // Ось вращения
    float angularSpeed;     // Радианы в секунду

    OrbitObject(std::vector<std::shared_ptr<GameComponent>> visuals, 
        std::shared_ptr<GameObject> targetObj, DirectX::XMFLOAT3 offsetVec,
        DirectX::XMFLOAT3 axis, float speed)
    {
        visual = visuals;
        target = targetObj;
        //center = XMLoadFloat3(&centerPos);
        offset = XMLoadFloat3(&offsetVec);
        rotationAxis = DirectX::XMVector3Normalize(XMLoadFloat3(&axis));
        angularSpeed = speed;
    }

    void Update(float deltaTime, float totalTime) override {
        if (!target) return;
        XMVECTOR currentRot = XMLoadFloat4(&rotation);

        // дельта-кватернион
        XMVECTOR deltaRot = XMQuaternionRotationAxis(selfAxis, selfSpeed * deltaTime);

        // накапливаем
        currentRot = XMQuaternionMultiply(currentRot, deltaRot);
        currentRot = XMQuaternionNormalize(currentRot);

        // сохраняем
        XMStoreFloat4(&rotation, currentRot);
        DirectX::XMVECTOR center = XMLoadFloat3(&target->position);
        // создаём кватернион вращения вокруг axis
        float angle = angularSpeed * totalTime;
        DirectX::XMVECTOR qRotation = DirectX::XMQuaternionRotationAxis(rotationAxis, angle);

        // вычисляем новую позицию
        DirectX::XMVECTOR newPos = center + DirectX::XMVector3Rotate(offset, qRotation);

        // записываем в трансформ
        DirectX::XMStoreFloat3(&position, newPos);
        for (auto& component : visual) {
            XMStoreFloat3(&component->transform.position, newPos);
        }
    }
};