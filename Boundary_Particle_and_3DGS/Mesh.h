#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <iostream>

#include <d3d11.h>
#include <windows.h>
#include <wrl.h> // ComPtr
#include <vector>

#include "MeshData.h"

namespace jhm {

using Microsoft::WRL::ComPtr;

// 같은 메쉬를 여러번 그릴 때 버퍼들을 재사용
struct Mesh {

    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;

    ComPtr<ID3D11Buffer> vertexConstantBuffer;
    ComPtr<ID3D11Buffer> geometryConstantBuffer;
    ComPtr<ID3D11Buffer> pixelConstantBuffer;

    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> textureResourceView;

    UINT m_indexCount = 0;

    MeshData m_meshData;
};
} // namespace hlab
