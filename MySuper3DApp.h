#pragma once

void SetupStateAndViewport(DirectXDevice& directXDevice, ID3D11RasterizerState* rastState, DisplayWin32& displayWin32, ShaderProgram& shaderProgram, ID3D11Buffer* ib, ID3D11Buffer*& vb, UINT  strides[1], UINT  offsets[1]);
