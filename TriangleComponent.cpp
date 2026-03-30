#include "TriangleComponent.h"
TriangleComponent::TriangleComponent(ID3D11Device* device, Vertex points[])
{
	TriangleComponent::points[0] = points[0];
	TriangleComponent::points[1] = points[1];
	TriangleComponent::points[2] = points[2];

	D3D11_BUFFER_DESC vertexBufDesc = {};
	vertexBufDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufDesc.CPUAccessFlags = 0;
	vertexBufDesc.MiscFlags = 0;
	vertexBufDesc.StructureByteStride = 0;
	vertexBufDesc.ByteWidth = sizeof(Vertex) * 3;

	D3D11_SUBRESOURCE_DATA vertexData = {};
	vertexData.pSysMem = this->points;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	device->CreateBuffer(&vertexBufDesc, &vertexData, &vb);

	D3D11_BUFFER_DESC indexBufDesc = {};
	indexBufDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufDesc.CPUAccessFlags = 0;
	indexBufDesc.MiscFlags = 0;
	indexBufDesc.StructureByteStride = 0;
	indexBufDesc.ByteWidth = sizeof(int) * std::size(indeces);

	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = indeces;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	device->CreateBuffer(&indexBufDesc, &indexData, &ib);


	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.ByteWidth = sizeof(VSConstants);
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;

	//device->CreateBuffer(&cbDesc, nullptr, &constantBuffer);

}
void TriangleComponent::Update(float deltaTime, float totalTime)
{

}
void TriangleComponent::Draw(ID3D11DeviceContext* context)
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);

	VSConstants data = {};
	data.offset = DirectX::XMFLOAT4(
		transform.position.x,
		transform.position.y,
		transform.position.z,
		0.0f
	);

	//context->UpdateSubresource(constantBuffer, 0, nullptr, &data, 0, 0);
	//context->VSSetConstantBuffers(0, 1, &constantBuffer);

    context->DrawIndexed(3, 0, 0);
}