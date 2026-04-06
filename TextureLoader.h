#pragma once
#include <d3d11.h>
#include <WICTextureLoader.h>
#include <string>

class TextureLoader
{
public:
    static ID3D11ShaderResourceView* LoadTexture(
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        const std::wstring& path
    )
    {
        ID3D11ShaderResourceView* texture = nullptr;

        DirectX::CreateWICTextureFromFile(
            device,
            context,
            path.c_str(),
            nullptr,
            &texture
        );

        return texture;
    }
};