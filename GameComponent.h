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
        XMFLOAT2 uv;
        XMFLOAT4 color;
    };
    struct Transform
    {
        DirectX::XMFLOAT3 offset = { 0.0f, 0.0f, 0.0f };
    };
    Transform transform;
    struct MeshData
    {
        std::vector<GameComponent::Vertex> vertices;
        std::vector<uint32_t> indices;
        ID3D11ShaderResourceView* texture = nullptr;
    };
    GameComponent(
        ID3D11Device* device,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices
    );
    GameComponent(
        ID3D11Device* device,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        ID3D11ShaderResourceView* texture
    );

    void Update(float deltaTime, float totalTime);
    void Draw(ID3D11DeviceContext* context);
    ID3D11ShaderResourceView* texture = nullptr;
    bool hasTexture = false;
    void SetTexture(ID3D11ShaderResourceView* tex)
    {
        texture = tex;
        hasTexture = (tex != nullptr);
    }

private:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    ID3D11Buffer* vb{};
    ID3D11Buffer* ib{};
};