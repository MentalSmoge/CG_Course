#include "GameComponent.h"
#include <iostream>

GameComponent::GameComponent(
    ID3D11Device* device,
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices
)
    : m_vertices(vertices), m_indices(indices)
{
    D3D11_BUFFER_DESC vertexBufDesc = {};
    vertexBufDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufDesc.CPUAccessFlags = 0;
    vertexBufDesc.MiscFlags = 0;
    vertexBufDesc.ByteWidth = sizeof(Vertex) * m_vertices.size();
    vertexBufDesc.StructureByteStride = sizeof(Vertex); // Обязательно

    D3D11_SUBRESOURCE_DATA vdata = {};
    vdata.pSysMem = m_vertices.data();
    vdata.SysMemPitch = 0;
    vdata.SysMemSlicePitch = 0;

    device->CreateBuffer(&vertexBufDesc, &vdata, &vb);

    D3D11_BUFFER_DESC indexBufDesc = {};
    indexBufDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufDesc.CPUAccessFlags = 0;
    indexBufDesc.MiscFlags = 0;
    indexBufDesc.StructureByteStride = 0;
    indexBufDesc.ByteWidth = UINT(sizeof(uint32_t) * m_indices.size());

    D3D11_SUBRESOURCE_DATA idata = {};
    idata.pSysMem = m_indices.data();
    idata.SysMemPitch = 0;
    idata.SysMemSlicePitch = 0;

    device->CreateBuffer(&indexBufDesc, &idata, &ib);
}

void GameComponent::Update(float deltaTime, float totalTime)
{

}

void GameComponent::Draw(ID3D11DeviceContext* context)
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->DrawIndexed(UINT(m_indices.size()), 0, 0);
}