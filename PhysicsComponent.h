#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <iostream>

using namespace DirectX;

struct PhysicsComponent {
    float mass = 1.0f;
    XMFLOAT3 size = { 10.0f, 10.0f, 10.0f };
    bool isActive;

    PhysicsComponent() :
        isActive(true) {
    }

    PhysicsComponent(XMFLOAT3 initSize) :
        size(initSize),
        isActive(true) {
    }
};