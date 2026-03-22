#include "ShaderProgram.h"
#include <iostream>

ShaderProgram::ShaderProgram(ID3D11Device* device,
    const wchar_t* file,
    const D3D11_INPUT_ELEMENT_DESC* layoutDesc,
    UINT layoutCount,
    const D3D_SHADER_MACRO* macros)
{
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;

    if (!CompileShader(file, "VSMain", "vs_5_0", nullptr, &vsBlob))
        return;

    if (!CompileShader(file, "PSMain", "ps_5_0", macros, &psBlob))
        return;
    auto res = device->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &vertexShader);

    if (FAILED(res))
        return;

    res = device->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &pixelShader);

    if (FAILED(res))
        return;

    res = device->CreateInputLayout(
        layoutDesc,
        layoutCount,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &inputLayout);

    vsBlob->Release();
    psBlob->Release();

    if (FAILED(res))
        return;

}

bool ShaderProgram::CompileShader(
    const wchar_t* file,
    const char* entry,
    const char* model,
    const D3D_SHADER_MACRO* macros,
    ID3DBlob** blob)
{
    ID3DBlob* errorBlob = nullptr;
	auto res = D3DCompileFromFile(
		file,
		macros /*macros*/,
		nullptr /*include*/,
		entry,
		model,
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		blob,
		&errorBlob);
    if (FAILED(res))
    {
        // If the shader failed to compile it should have written something to the error message.
        if (errorBlob)
        {
            std::cout << (char*)errorBlob->GetBufferPointer() << std::endl;
            errorBlob->Release();
        }
        // If there was  nothing in the error message then it simply could not find the shader file itself.
        //else std::cout << L"Missing Shader File\n";
        return false;
    }

    if (errorBlob)
        errorBlob->Release();

    return true;

}