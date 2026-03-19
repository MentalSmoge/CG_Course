#pragma once
#include <d3dcompiler.h>
#include <d3d11.h>
#include <wrl.h>
class ShaderProgram
{
public:
	ShaderProgram(
        ID3D11Device* device,
        const wchar_t* file,
        const D3D11_INPUT_ELEMENT_DESC* layoutDesc,
        UINT layoutCount,
        const D3D_SHADER_MACRO* macros);

    bool CompileShader(
        const wchar_t* file,
        const char* entry,
        const char* model,
        const D3D_SHADER_MACRO* macros,
        ID3DBlob** blob);
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
};