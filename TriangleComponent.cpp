#include "TriangleComponent.h"
TriangleComponent::TriangleComponent(ID3D11Device* device, Vertex points[])
{
	TriangleComponent::points[0] = points[0];
	TriangleComponent::points[1] = points[1];
	TriangleComponent::points[2] = points[2];
}
void TriangleComponent::Draw(ID3D11DeviceContext* context)
{
    UINT strides[] = { sizeof(Vertex) };
    UINT offsets[] = { 0 };

    //// Установка буферов
    //context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), strides, offsets);
    //context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    //// Отрисовка
    //context->DrawIndexed(6, 0, 0);
}