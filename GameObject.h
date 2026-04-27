#pragma once
#include "GameComponent.h"
#include <vector>
#include <d3d11.h>
#include <memory>
#include "PhysicsComponent.h"
class GameObject : public std::enable_shared_from_this<GameObject>

{
public:
	struct Transform
	{
		DirectX::XMFLOAT3 position = { 0,0,0 };
		DirectX::XMFLOAT3 scale = { 1,1,1 };
		DirectX::XMFLOAT4 rotation = { 0,0,0,1 };
	};
    struct MaterialBuffer
    {
        XMFLOAT3 ambient;
        float pad1;
        XMFLOAT3 diffuse;
        float pad2;
        XMFLOAT3 specular;
        float shininess;
    };
    bool hasPointLight = false;
    DirectX::XMFLOAT3 pointLightColor = { 1.0f, 0.8f, 0.5f };
    float pointLightRange = 20.0f;
    float pointLightIntensity = 2.0f;

    void EnablePointLight(bool enable) { hasPointLight = enable; }
    bool isProjectile = false;              
    DirectX::XMFLOAT3 velocity = { 0,0,0 };    
    float gravity = -9.8f;                 
    std::vector<std::shared_ptr<GameObject>> attachedObjects;
    void Shoot(DirectX::XMFLOAT3 direction, float speed);

    MaterialBuffer mb{};
    DirectX::XMFLOAT3 localPosition = { 0,0,0 };
    DirectX::XMFLOAT3 localScale = { 1,1,1 };
    DirectX::XMFLOAT4 localRotation = { 0,0,0,1 };
    std::shared_ptr<GameObject> parent = nullptr;
	Transform transform;
	std::shared_ptr<GameComponent> visual;
	PhysicsComponent physics{};
	BoundingBox boundingBox{};
	void move(DirectX::XMFLOAT3 direction);
	void move_teleport(DirectX::XMFLOAT3 position);
	GameObject(std::shared_ptr<GameComponent> visuals);
    GameObject(std::shared_ptr<GameComponent> visuals, XMFLOAT3 size, XMFLOAT3 pos);
	GameObject() = default;
	virtual void Update(float deltaTime, float totalTime);
	void Draw(ID3D11DeviceContext* context);


	void UpdateBoundingBox();
    bool CheckCollision(std::shared_ptr<GameObject> other);
    void AttachToParent(std::shared_ptr<GameObject> newParent);
	void Rotate(XMFLOAT3 axis, float angle);
	void SetScale(float s);
    ID3D11Buffer* bboxVB = nullptr;
    ID3D11Buffer* bboxIB = nullptr;
    bool bboxInitialized = false;
    void InitBoundingBoxBuffers(ID3D11Device* device)
    {
        if (bboxInitialized) return;

        std::vector<GameComponent::Vertex> verts(8);

        D3D11_BUFFER_DESC vbd = {};
        vbd.Usage = D3D11_USAGE_DYNAMIC;
        vbd.ByteWidth = sizeof(GameComponent::Vertex) * 8;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        device->CreateBuffer(&vbd, nullptr, &bboxVB);

        uint32_t indices[] =
        {
            0,1, 1,2, 2,3, 3,0,
            4,5, 5,6, 6,7, 7,4,
            0,4, 1,5, 2,6, 3,7
        };

        D3D11_BUFFER_DESC ibd = {};
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        ibd.ByteWidth = sizeof(indices);
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA idata = {};
        idata.pSysMem = indices;

        device->CreateBuffer(&ibd, &idata, &bboxIB);

        bboxInitialized = true;
    }
    void UpdateBoundingBoxVertices(ID3D11DeviceContext* context)
    {
        DirectX::XMFLOAT3 corners[8];
        boundingBox.GetCorners(corners);

        D3D11_MAPPED_SUBRESOURCE mapped;
        context->Map(bboxVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

        auto* v = (GameComponent::Vertex*)mapped.pData;

        for (int i = 0; i < 8; i++)
        {
            v[i].pos = { 
                (corners[i].x - transform.position.x - visual->transform.offset.x) / transform.scale.x,
                (corners[i].y - transform.position.y - visual->transform.offset.y) / transform.scale.y,
                (corners[i].z - transform.position.z - visual->transform.offset.z) / transform.scale.z, 1
            };
            v[i].normal = { 0,0,0 };
            v[i].uv = { 0,0 };
            v[i].color = { 1,0,0,1 };
        }

        context->Unmap(bboxVB, 0);
    }
    void DrawBoundingBox(ID3D11DeviceContext* context)
    {
        UINT stride = sizeof(GameComponent::Vertex);
        UINT offset = 0;

        context->IASetVertexBuffers(0, 1, &bboxVB, &stride, &offset);
        context->IASetIndexBuffer(bboxIB, DXGI_FORMAT_R32_UINT, 0);

        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

        context->DrawIndexed(24, 0, 0);
    }
};