#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

class GameComponent
{
public:
    struct Vertex
    {
        XMFLOAT4 pos;
        XMFLOAT3 normal;
        XMFLOAT4 color;
    };
    struct Transform
    {
        DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
    };
    Transform transform;
    struct MeshData
    {
        std::vector<GameComponent::Vertex> vertices;
        std::vector<uint32_t> indices;
    };
    GameComponent(
        ID3D11Device* device,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices
    );

    void Update(float deltaTime, float totalTime);
    void Draw(ID3D11DeviceContext* context);

private:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    ID3D11Buffer* vb{};
    ID3D11Buffer* ib{};
};