#pragma once
#include <directxmath.h>
#include <d3d11.h>
#include <iterator>
class TriangleComponent
{
public:
	struct Vertex
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 color;
	};
	TriangleComponent(ID3D11Device* device, Vertex points[]);
	void Update(float deltaTime, float totalTime);
	void Draw(ID3D11DeviceContext* context);
	Vertex points[3];
private:
	int indeces[3] = { 0, 1, 2 };
	ID3D11Buffer* vb{};
	ID3D11Buffer* ib{};
};