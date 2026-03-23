#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <iostream>

using namespace DirectX;

struct PhysicsComponent {
    XMFLOAT3 velocity{};
    float mass = 1.0f;
    XMFLOAT2 size = { 0.0f, 0.0f };
    bool isActive;

    PhysicsComponent() :
        velocity(0, 0, 0),
        isActive(true) {
    }

    PhysicsComponent(XMFLOAT3 initVelocity, XMFLOAT2 initSize) :
        velocity(initVelocity),
        size(initSize),
        isActive(true) {
        std::cout << size.x << "\n";
    }
};