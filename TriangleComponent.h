#pragma once
#include <directxmath.h>
#include <d3d11.h>
#include <iterator>
class TriangleComponent
{
public:
	struct Transform
	{
		DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	};
	struct Vertex
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 color;
	};
	struct VSConstants
	{
		DirectX::XMFLOAT4 offset;
	};
	TriangleComponent(ID3D11Device* device, Vertex points[]);
	void Update(float deltaTime, float totalTime);
	void Draw(ID3D11DeviceContext* context);
	Transform transform;
private:
	int indeces[3] = { 0, 1, 2 };
	ID3D11Buffer* vb{};
	ID3D11Buffer* ib{};
	ID3D11Buffer* constantBuffer = nullptr;
	Vertex points[3];
};