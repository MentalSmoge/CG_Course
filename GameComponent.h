#pragma once
#include "TriangleComponent.h"

class GameComponent : public TriangleComponent
{
public:
	GameComponent(ID3D11Device* device, Vertex points[]);
};