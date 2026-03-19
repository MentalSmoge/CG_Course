#include "GameComponent.h"
GameComponent::GameComponent(ID3D11Device* device, Vertex points[])
    : TriangleComponent(device, points)
{
}