
#include "BasicMeshGroup.h"
#include "GeometryGenerator.h"

namespace jhm {
void BasicMeshGroup::Initialize(ComPtr<ID3D11Device> &device,
                           const std::string &basePath,
                           const std::string &filename) {

    auto meshes = GeometryGenerator::ReadFromFile(basePath, filename);

    Initialize(device, meshes);
}

void BasicMeshGroup::Initialize(ComPtr<ID3D11Device> &device,
                           const std::vector<MeshData> &meshes) {

    // Sampler 만들기
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    device->CreateSamplerState(&sampDesc, m_samplerState.GetAddressOf());

    // ConstantBuffer 만들기
    m_basicVertexConstantData.model = Matrix();
    m_basicVertexConstantData.view = Matrix();
    m_basicVertexConstantData.projection = Matrix();

    m_basicGeometryConstantData.model = Matrix();
    m_basicGeometryConstantData.view = Matrix();
    m_basicGeometryConstantData.projection = Matrix();

    D3D11Utils::CreateConstantBuffer(device, m_basicVertexConstantData,
                                     m_vertexConstantBuffer);
    D3D11Utils::CreateConstantBuffer(device, m_basicGeometryConstantData,
                                     m_geometryConstantBuffer);
    D3D11Utils::CreateConstantBuffer(device, m_basicPixelConstantData,
                                     m_pixelConstantBuffer);

    for (const auto &meshData : meshes) {
        auto newMesh = std::make_shared<Mesh>();

        // 복사대신 shared_ptr 사용 고려
        newMesh->m_meshData = meshData;

        D3D11Utils::CreatePBDVertexBuffer(device, meshData.vertices,
                                       newMesh->vertexBuffer);
        newMesh->m_indexCount = UINT(meshData.indices.size());
        D3D11Utils::CreatePBDIndexBuffer(device, meshData.indices,
                                      newMesh->indexBuffer);

        if (!meshData.textureFilename.empty()) {

            std::cout << meshData.textureFilename << std::endl;
            D3D11Utils::CreateTexture(device, meshData.textureFilename,
                                      newMesh->texture,
                                      newMesh->textureResourceView);
        }

        newMesh->vertexConstantBuffer = m_vertexConstantBuffer;
        newMesh->geometryConstantBuffer = m_geometryConstantBuffer;
        newMesh->pixelConstantBuffer = m_pixelConstantBuffer;

        this->m_meshes.push_back(newMesh);
    }
    vector<D3D11_INPUT_ELEMENT_DESC> basicInputElements = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 3,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    vector<D3D11_INPUT_ELEMENT_DESC> GSInputElements = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"SCALE", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"ROTATION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 3 + 4 * 3,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"SH", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3 + 4 * 3 + 4 * 4,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"OPACITY", 0, DXGI_FORMAT_R32_FLOAT, 0, 232,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 232 + 4,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 232 + 4 + 4 * 3,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    D3D11Utils::CreateVertexShaderAndInputLayout(
        device, L"GSVertexShader.hlsl", GSInputElements,
        m_basicVertexShader, m_GSInputLayout);
    D3D11Utils::CreateGeometryShader(device, L"GSGeometryShader.hlsl",
                                     m_basicGeometryShader);
    D3D11Utils::CreatePixelShader(device, L"GSPixelShader.hlsl",
                                  m_basicPixelShader);

    // 노멀 벡터 그리기
    m_normalLines = std::make_shared<Mesh>();

    std::vector<Vertex> normalVertices;
    std::vector<uint32_t> normalIndices;

    // 여러 메쉬의 normal 들을 하나로 합치기
    size_t offset = 0;
    for (const auto &meshData : meshes) {
        for (size_t i = 0; i < meshData.vertices.size(); i++) {

            auto v = meshData.vertices[i];

            v.texcoord.x = 0.0f; // 시작점 표시
            normalVertices.push_back(v);

            v.texcoord.x = 1.0f; // 끝점 표시
            normalVertices.push_back(v);

            normalIndices.push_back(uint32_t(2 * (i + offset)));
            normalIndices.push_back(uint32_t(2 * (i + offset) + 1));
        }
        offset += meshData.vertices.size();
    }

    D3D11Utils::CreatePBDVertexBuffer(device, normalVertices,
                                   m_normalLines->vertexBuffer);
    m_normalLines->m_indexCount = UINT(normalIndices.size());
    D3D11Utils::CreatePBDIndexBuffer(device, normalIndices,
                                  m_normalLines->indexBuffer);

    D3D11Utils::CreateVertexShaderAndInputLayout(
        device, L"NormalVertexShader.hlsl", GSInputElements,
        m_normalVertexShader, m_basicInputLayout);
    D3D11Utils::CreatePixelShader(device, L"NormalPixelShader.hlsl",
                                  m_normalPixelShader);

    D3D11Utils::CreateConstantBuffer(device, m_normalVertexConstantData,
                                     m_normalVertexConstantBuffer);
}

void BasicMeshGroup::UpdateConstantBuffers(ComPtr<ID3D11Device> &device,
                                      ComPtr<ID3D11DeviceContext> &context) {

    D3D11Utils::UpdateConstantBuffer(device, context, m_basicVertexConstantData,
                             m_vertexConstantBuffer);
    D3D11Utils::UpdateConstantBuffer(device, context, m_basicGeometryConstantData,
                                     m_geometryConstantBuffer);
    D3D11Utils::UpdateConstantBuffer(device, context, m_basicPixelConstantData,
                             m_pixelConstantBuffer);

    // 노멀 벡터 그리기
    if (m_drawNormals && m_drawNormalsDirtyFlag) {
        D3D11Utils::UpdateConstantBuffer(device, context,
                                         m_normalVertexConstantData,
                                 m_normalVertexConstantBuffer);
        m_drawNormalsDirtyFlag = false;
    }
}

void BasicMeshGroup::UpdateVertexBuffers(ComPtr<ID3D11Device> &device,
                                         ComPtr<ID3D11DeviceContext> &context) {
    for (auto &mesh : m_meshes) {

        D3D11Utils::UpdateBuffer(device, context,
                                         mesh->m_meshData.vertices,
                                         mesh->vertexBuffer);
    }
}
void BasicMeshGroup::UpdateIndexBuffers(ComPtr<ID3D11Device> &device,
                                         ComPtr<ID3D11DeviceContext> &context) {
    for (auto &mesh : m_meshes) {

        D3D11Utils::UpdateBuffer(device, context, mesh->m_meshData.indices,
                                 mesh->indexBuffer);
        mesh->m_indexCount = mesh->m_meshData.indices.size();
    }
}
template <typename T_DATA>
void BasicMeshGroup::UpdateNormalLinesVertexBuffers(
    ComPtr<ID3D11Device> &device, ComPtr<ID3D11DeviceContext> &context,
    const T_DATA &bufferData, ComPtr<ID3D11Buffer> &buffer) {
    if (!buffer) {
        std::cout << "UpdateVertexBuffer() buffer was not initialized."
                  << std::endl;
    }

    D3D11Utils::UpdateBuffer(device, context, bufferData, buffer);
}

template <typename T_DATA>
void BasicMeshGroup::UpdateNormalLinesIndexBuffers(
    ComPtr<ID3D11Device> &device, ComPtr<ID3D11DeviceContext> &context,
    const T_DATA &bufferData, ComPtr<ID3D11Buffer> &buffer) {
    if (!buffer) {
        std::cout << "UpdateIndexBuffer() buffer was not initialized.\n";
        return;
    }
    D3D11Utils::UpdateBuffer(device, context, bufferData, buffer);
}

void BasicMeshGroup::Render(ComPtr<ID3D11DeviceContext> &context) {

    context->VSSetShader(m_basicVertexShader.Get(), 0, 0);
    context->GSSetShader(m_basicGeometryShader.Get(), 0, 0);
    context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
    context->PSSetShader(m_basicPixelShader.Get(), 0, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    for (const auto &mesh : m_meshes) {

        context->VSSetConstantBuffers(
            0, 1, mesh->vertexConstantBuffer.GetAddressOf());
        context->GSSetConstantBuffers(
            0, 1, mesh->geometryConstantBuffer.GetAddressOf());
        // 물체 렌더링할 때 큐브맵도 같이 사용
        ID3D11ShaderResourceView *resViews[3] = {
            mesh->textureResourceView.Get(), m_diffuseResView.Get(),
            m_specularResView.Get()};
        context->PSSetShaderResources(0, 3, resViews);

        context->PSSetConstantBuffers(0, 1,
                                      mesh->pixelConstantBuffer.GetAddressOf());

        context->IASetInputLayout(m_GSInputLayout.Get());
        context->IASetVertexBuffers(0, 1, mesh->vertexBuffer.GetAddressOf(),
                                    &stride, &offset);
        context->IASetIndexBuffer(mesh->indexBuffer.Get(), DXGI_FORMAT_R32_UINT,
                                  0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        context->DrawIndexed(mesh->m_indexCount, 0, 0);
    }

    // 노멀 벡터 그리기
    if (m_drawNormals) {
        context->GSSetShader(nullptr, nullptr, 0);
        context->VSSetShader(m_normalVertexShader.Get(), 0, 0);
        ID3D11Buffer *pptr[2] = {m_vertexConstantBuffer.Get(),
                                 m_normalVertexConstantBuffer.Get()};
        context->VSSetConstantBuffers(0, 2, pptr);
        context->PSSetShader(m_normalPixelShader.Get(), 0, 0);
        context->IASetInputLayout(m_basicInputLayout.Get());
        context->IASetVertexBuffers(
            0, 1, m_normalLines->vertexBuffer.GetAddressOf(), &stride, &offset);
        context->IASetIndexBuffer(m_normalLines->indexBuffer.Get(),
                                  DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        context->DrawIndexed(m_normalLines->m_indexCount, 0, 0);
    }
}

void BasicMeshGroup::InitParticles()
{
    float d = m_particle_distance;

    for (auto& mesh : m_meshes)
    {
        MeshData &meshData = mesh->m_meshData;
        for (auto &t : meshData.triangles) {

            t.shortEdgeIndex = -1;
            t.innerParticlesIndices.clear();
            t.lineParticles.clear();

            Edge &e0 = meshData.edges[t.edgeIndices[0]];
            Edge &e1 = meshData.edges[t.edgeIndices[1]];
            Edge &e2 = meshData.edges[t.edgeIndices[2]];

            EdgeSampling(meshData, e0, d);
            EdgeSampling(meshData, e1, d);
            EdgeSampling(meshData, e2, d);

            InnerSampling(meshData, t, d);
        }
    }
}

void BasicMeshGroup::UpdateParticles()
{
    float d = m_particle_distance;

    for (auto &mesh : m_meshes) {

        MeshData &meshData = mesh->m_meshData;

        for (auto &e : meshData.edges)
            e.visited = false;

        for (auto &t : meshData.triangles) {

            Edge &e0 = meshData.edges[t.edgeIndices[0]];
            Edge &e1 = meshData.edges[t.edgeIndices[1]];
            Edge &e2 = meshData.edges[t.edgeIndices[2]];

            EdgeSampling(meshData, e0, d);
            EdgeSampling(meshData, e1, d);
            EdgeSampling(meshData, e2, d);

            InnerSampling(meshData,t, d);
        }
    }
}

void BasicMeshGroup::EdgeSampling(MeshData& meshData, Edge& e, float d)
{
    if (e.visited == false) {
        int index0 = e.index0;
        int index1 = e.index1;

        Vector3 pos0 = meshData.vertices[index0].position;
        Vector3 pos1 = meshData.vertices[index1].position;

        float l = (pos0 - pos1).Length();
        int n = (int)std::floor(l / d);

        int& numEdgeParticles = e.numEdgeParticles;
        // Initial
        if (numEdgeParticles == -1) {
            numEdgeParticles = n;

            Vector3 p = (pos0 - pos1) / numEdgeParticles;

            Vertex v;
            for (int i = 1; i < numEdgeParticles; ++i) {
                v.position = pos1 + p * i;
                v.normal = (meshData.vertices[index1].normal * (n - i) +
                            meshData.vertices[index0].normal * i);
                v.normal.Normalize();
                v.texcoord = (meshData.vertices[index1].texcoord * (n - i) +
                              meshData.vertices[index0].texcoord * i);

                e.edgeIndices.push_back(meshData.vertices.size());
                meshData.indices.push_back(meshData.vertices.size());
                meshData.vertices.push_back(v);
            }
        }

        // update
        else if (numEdgeParticles == n) {
            Vector3 p = (pos0 - pos1) / numEdgeParticles;

            for (int i = 1; i < numEdgeParticles; ++i) {
                int index = e.edgeIndices[i - 1];
                meshData.vertices[index].position = pos1 + p * i;
                meshData.vertices[index].normal =
                    (meshData.vertices[index1].normal * (numEdgeParticles - i) +
                     meshData.vertices[index0].normal * i);
                meshData.vertices[index].normal.Normalize();
                meshData.vertices[index].texcoord =
                    (meshData.vertices[index1].texcoord *
                         (numEdgeParticles - i) +
                     meshData.vertices[index0].texcoord * i);
            }
        }

        // resampling
        else {
            Vector3 p = (pos0 - pos1) / n;

            // upsampling
            if (n > numEdgeParticles) {
                for (int i = 1; i < n; ++i) {
                    if (i < numEdgeParticles) {
                        int index = e.edgeIndices[i - 1];
                        meshData.vertices[index].position = pos1 + p * i;
                        meshData.vertices[index].normal =
                            (meshData.vertices[index1].normal * (n - i) +
                             meshData.vertices[index0].normal * i);
                        meshData.vertices[index].normal.Normalize();
                        meshData.vertices[index].texcoord =
                            (meshData.vertices[index1].texcoord * (n - i) +
                             meshData.vertices[index0].texcoord * i);
                    } else {

                        Vertex v;
                        v.position = pos1 + p * i;
                        v.normal =
                            (meshData.vertices[index1].normal * (n - i) +
                             meshData.vertices[index0].normal * i);
                        v.normal.Normalize();
                        v.texcoord =
                            (meshData.vertices[index1].texcoord * (n - i) +
                             meshData.vertices[index0].texcoord * i);

                        e.edgeIndices.push_back(meshData.vertices.size());
                        meshData.indices.push_back(
                            meshData.vertices.size());
                        meshData.vertices.push_back(v);
                    }
                }
            }
            // downsampling
            else {
                for (int i = 1; i < numEdgeParticles; ++i) {
                    if (i < n) {
                        int index = e.edgeIndices[i - 1];
                        meshData.vertices[index].position = pos1 + p * i;
                        meshData.vertices[index].normal =
                            (meshData.vertices[index1].normal * (n - i) +
                             meshData.vertices[index0].normal * i);
                        meshData.vertices[index].normal.Normalize();
                        meshData.vertices[index].texcoord =
                            (meshData.vertices[index1].texcoord * (n - i) +
                             meshData.vertices[index0].texcoord * i);
                    } else {
                        // downsampling된 vertex 삭제 대신 scale -> 0
                        int index = e.edgeIndices.back();
                        e.edgeIndices.pop_back();
                        meshData.vertices[index].scale = {0.0f, 0.0f,
                                                               0.0f};
                    }
                }
            }
            numEdgeParticles = n;
        }

        e.visited = true;
    }
}

void BasicMeshGroup::InnerSampling(MeshData& meshData, Triangle& t, float d)
{
    // inner particle
    int index0 = t.vertexIndices[0];
    int index1 = t.vertexIndices[1];
    int index2 = t.vertexIndices[2];

    Vector3 pos0 = meshData.vertices[index0].position;
    Vector3 pos1 = meshData.vertices[index1].position;
    Vector3 pos2 = meshData.vertices[index2].position;

    Vector3 normal0 = meshData.vertices[index0].normal;
    Vector3 normal1 = meshData.vertices[index1].normal;
    Vector3 normal2 = meshData.vertices[index2].normal;

    float l0 = (pos0 - pos1).Length();
    float l1 = (pos1 - pos2).Length();
    float l2 = (pos2 - pos0).Length();

    Vector3 longEdge;
    Vector3 middleEdge;
    Vector3 shortEdge;
    Vector3 middleStart;
    Vector3 longStart;
    float longEdgeLength = std::max(std::max(l0, l1), l2);
    float shortEdgeLength = std::min(std::min(l0, l1), l2);
    int shortEdgeIndex;

    if (longEdgeLength == l0) {
        if (shortEdgeLength == l1) {
            shortEdge = pos2 - pos1;
            shortEdgeIndex = t.edgeIndices[1];
            middleEdge = pos0 - pos2;
            longEdge = pos0 - pos1;
            middleStart = pos2;
            longStart = pos1;
        } else {
            shortEdge = pos2 - pos0;
            shortEdgeIndex = t.edgeIndices[2];
            middleEdge = pos1 - pos2;
            longEdge = pos1 - pos0;
            middleStart = pos2;
            longStart = pos0;
        }
    } else if (longEdgeLength == l1) {
        if (shortEdgeLength == l0) {
            shortEdge = pos0 - pos1;
            shortEdgeIndex = t.edgeIndices[0];
            middleEdge = pos2 - pos0;
            longEdge = pos2 - pos1;
            middleStart = pos0;
            longStart = pos1;
        } else {
            shortEdge = pos0 - pos2;
            shortEdgeIndex = t.edgeIndices[2];
            middleEdge = pos1 - pos0;
            longEdge = pos1 - pos2;
            middleStart = pos0;
            longStart = pos2;
        }
    } else {
        if (shortEdgeLength == l0) {
            shortEdge = pos1 - pos0;
            shortEdgeIndex = t.edgeIndices[0];
            middleEdge = pos2 - pos1;
            longEdge = pos2 - pos0;
            middleStart = pos1;
            longStart = pos0;
        } else {
            shortEdge = pos1 - pos2;
            shortEdgeIndex = t.edgeIndices[1];
            middleEdge = pos0 - pos1;
            longEdge = pos0 - pos2;
            middleStart = pos1;
            longStart = pos2;
        }
    }

    Vector3 s = shortEdge.Cross(longEdge.Cross(shortEdge));
    s.Normalize();

    float normalLength = s.Dot(longEdge);

    int numNormalParticles = (int)std::floor(normalLength / d);
    int numShortEdgeParticles = (int)std::floor(shortEdgeLength / d);

    numNormalParticles = std::max(1, numNormalParticles);
    numShortEdgeParticles = std::max(1, numShortEdgeParticles);

    Vector3 middleEdgeUnitVector = middleEdge / numNormalParticles;
    Vector3 longEdgeUnitVector = longEdge / numNormalParticles;

    // Initial
    if (t.shortEdgeIndex == -1) {
        t.shortEdgeIndex = shortEdgeIndex;
        t.numNormalParticles = numNormalParticles;
        t.numShortEdgeParticles = numShortEdgeParticles;
        Vertex inner;

        for (int i = 1; i < numNormalParticles; ++i) {
            Vector3 i1 = middleStart + middleEdgeUnitVector * i;
            Vector3 i2 = longStart + longEdgeUnitVector * i;
            float lineLength = (i1 - i2).Length();
            int lineParticles = (int)std::floor(lineLength / d);
            lineParticles = std::max(1, lineParticles);
            t.lineParticles.push_back(lineParticles);
            Vector3 d = (i1 - i2) / lineParticles;

            for (int j = 1; j < lineParticles; ++j) {
                inner.position = i2 + d * j;

                // normal barycentric interpolation
                float s2 =
                    ((pos0 - inner.position).Cross(pos1 - inner.position))
                        .Length();
                float s1 =
                    ((pos2 - inner.position).Cross(pos0 - inner.position))
                        .Length();
                float s0 =
                    ((pos1 - inner.position).Cross(pos2 - inner.position))
                        .Length();

                inner.normal = normal0 * s0 + normal1 * s1 + normal2 * s2;

                inner.normal.Normalize();

                t.innerParticlesIndices.push_back(
                    meshData.vertices.size());
                meshData.indices.push_back(meshData.vertices.size());
                meshData.vertices.push_back(inner);
            }
        }
    }

    // resampling
    else if (t.shortEdgeIndex != shortEdgeIndex ||
             numNormalParticles * numShortEdgeParticles !=
                 t.numNormalParticles * t.numShortEdgeParticles) {
        t.shortEdgeIndex = shortEdgeIndex;

        t.numNormalParticles = numNormalParticles;
        t.numShortEdgeParticles = numShortEdgeParticles;
        Vertex inner;
        t.lineParticles.clear();

        int count = 0;
        for (int i = 1; i < numNormalParticles; ++i) {
            Vector3 i1 = middleStart + middleEdgeUnitVector * i;
            Vector3 i2 = longStart + longEdgeUnitVector * i;
            float l = (i1 - i2).Length();
            int lineParticles = (int)std::floor(l / d);
            t.lineParticles.push_back(lineParticles);
            Vector3 d = (i1 - i2) / lineParticles;

            for (int j = 1; j < lineParticles; ++j) {
                if (count < t.innerParticlesIndices.size()) {
                    int idx = t.innerParticlesIndices[count];
                    Vector3 newPosition = i2 + d * j;
                    meshData.vertices[idx].position = newPosition;

                    // normal barycentric interpolation
                    float s2 = ((pos0 - newPosition).Cross(pos1 - newPosition))
                                   .Length();
                    float s1 = ((pos2 - newPosition).Cross(pos0 - newPosition))
                                   .Length();
                    float s0 = ((pos1 - newPosition).Cross(pos2 - newPosition))
                                   .Length();

                    meshData.vertices[idx].normal =
                        normal0 * s0 + normal1 * s1 + normal2 * s2;

                    meshData.vertices[idx].normal.Normalize();
                } else {
                    inner.position = i2 + d * j;

                    // normal barycentric interpolation
                    float s2 =
                        ((pos0 - inner.position).Cross(pos1 - inner.position))
                            .Length();
                    float s1 =
                        ((pos2 - inner.position).Cross(pos0 - inner.position))
                            .Length();
                    float s0 =
                        ((pos1 - inner.position).Cross(pos2 - inner.position))
                            .Length();

                    inner.normal = normal0 * s0 + normal1 * s1 + normal2 * s2;

                    inner.normal.Normalize();

                    t.innerParticlesIndices.push_back(
                        meshData.vertices.size());
                    meshData.indices.push_back(meshData.vertices.size());
                    meshData.vertices.push_back(inner);
                }
                ++count;
            }
        }
        // downsampling된 vertex 삭제 대신 scale -> 0
        while (count < t.innerParticlesIndices.size()) {
            int idx = t.innerParticlesIndices.back();
            t.innerParticlesIndices.pop_back();
            meshData.vertices[idx].scale = {0.0f, 0.0f, 0.0f};
        }
    }

    // update
    else {
        int count = 0;
        for (int i = 1; i < t.numNormalParticles; ++i) {
            Vector3 i1 = middleStart + middleEdgeUnitVector * i;
            Vector3 i2 = longStart + longEdgeUnitVector * i;
            float l = (i1 - i2).Length();

            int lineParticles = t.lineParticles[i - 1];
            Vector3 d = (i1 - i2) / lineParticles;

            for (int j = 1; j < lineParticles; ++j) {
                int idx = t.innerParticlesIndices[count];
                Vector3 newPosition = i2 + d * j;
                meshData.vertices[idx].position = newPosition;

                // normal barycentric interpolation
                float s2 =
                    ((pos0 - newPosition).Cross(pos1 - newPosition)).Length();
                float s1 =
                    ((pos2 - newPosition).Cross(pos0 - newPosition)).Length();
                float s0 =
                    ((pos1 - newPosition).Cross(pos2 - newPosition)).Length();

                meshData.vertices[idx].normal =
                    normal0 * s0 + normal1 * s1 + normal2 * s2;

                meshData.vertices[idx].normal.Normalize();
                ++count;
            }
        }
    }
}

void BasicMeshGroup::ApplyExtForces(float dt)
{
    Vector3 gravity(0.0f, -9.8f, 0.0f);
    float damping = 0.59f;
    // 초기(평형) 상태(물체의 무게 * 중력  = 부력) 가정
    //float density = 1 / m_volume;
    float dragCoefficient = 0.47f;
    float dragArea = 10.0f;
    for (auto &mesh : m_meshes) {
        MeshData &meshData = mesh->m_meshData;
        for (int i = 0; i < meshData.verticesPBD.size(); ++i) {
            Vertex v = meshData.vertices[i];
            VertexPBD vPBD = meshData.verticesPBD[i];
            Vector3 pos = v.position;
            Vector3 vel = vPBD.velocity;
            Vector3 velNorm = vPBD.velocity;
            velNorm.Normalize();

            ////중력 계산
            // vel += gravity * dt;
            ////부력 계산
            // Vector3 buoyancy = -gravity * m_cur_volume * density *
            // v2.invMass; vel += buoyancy * dt;
            ////항력 계산
            // Vector3 drag = - 0.5 * density * vel.Length() * vel.Length() *
            // dragArea * dragCoefficient * velNorm * v2.invMass; vel += drag *
            // dt;

            vel *= damping;
            vPBD.newPosition = pos + vel * dt;

            meshData.verticesPBD[i].velocity = vel;
            meshData.verticesPBD[i].newPosition = vPBD.newPosition;
        }
    }
}

void BasicMeshGroup::ProjectDistanceConstraints()
{
    for (auto& mesh : m_meshes)
    {
        MeshData &meshData = mesh->m_meshData;
        for (auto& e : meshData.edges)
        {
            this->ProjectDistanceConstraint(meshData, e);
        }
    }
}

void BasicMeshGroup::ProjectDistanceConstraint(MeshData &meshData, Edge& e) {
    float k = 0.9f;

    UINT index0 = e.index0;
    UINT index1 = e.index1;
    float restLength = e.restLength;

        if (meshData.verticesPBD.size() <= index1)
        std::cout << meshData.verticesPBD.size() << " " << index1 << std::endl;

    Vector3 pos1 = meshData.verticesPBD[index0].newPosition;
    Vector3 pos2 = meshData.verticesPBD[index1].newPosition;
    float invMass1 = meshData.verticesPBD[index0].invMass;
    float invMass2 = meshData.verticesPBD[index1].invMass;

    float c_p1p2 = (pos1 - pos2).Length() - restLength;

    Vector3 dp1 = (pos1 - pos2);
    Vector3 dp2 = (pos1 - pos2);
    dp1.Normalize();
    dp2.Normalize();
    dp1 *= -invMass1 / (invMass1 + invMass2) * c_p1p2;
    dp2 *= invMass2 / (invMass1 + invMass2) * c_p1p2;
    pos1 += k * dp1;
    pos2 += k * dp2;

    meshData.verticesPBD[index0].newPosition = pos1;
    meshData.verticesPBD[index1].newPosition = pos2;


    // 누적 후, 한 번에 업데이트
    // meshData.verticesPBD[index0].gradPosition += k * dp1;
    // meshData.verticesPBD[index1].gradPosition += k * dp2;
    
}

void BasicMeshGroup::SolveOverpressureConstraints()
{
    for (auto &mesh : m_meshes) {
        MeshData &meshData = mesh->m_meshData;
        float constraintScale = this->computeVolumeConstraintScaling(meshData); 
        
        for (auto &t : meshData.triangles) {

            this->solveOverpressureConstraint(meshData, t, constraintScale);
        }
        
    }
}

float BasicMeshGroup::computeVolumeConstraintScaling(MeshData& meshData)
{
    std::vector<Vector3> grad(meshData.verticesPBD.size(),
                              Vector3(0.0, 0.0, 0.0));
    float curVolume = 0.0f;
    float gradSum = 0.0f;
    for (auto &t : meshData.triangles) {
        int index0 = t.vertexIndices[0];
        int index1 = t.vertexIndices[1];
        int index2 = t.vertexIndices[2];

        Vector3 pos0 = meshData.verticesPBD[index0].newPosition;
        Vector3 pos1 = meshData.verticesPBD[index1].newPosition;
        Vector3 pos2 = meshData.verticesPBD[index2].newPosition;

        grad[index0] += (pos1 - pos0).Cross(pos2 - pos0);
        grad[index1] += (pos2 - pos1).Cross(pos0 - pos1);
        grad[index2] += (pos0 - pos2).Cross(pos1 - pos2);

        curVolume += pos0.Cross(pos1).Dot(pos2);
    }

    for (int i = 0; i < grad.size(); ++i) {
        gradSum += grad[i].LengthSquared() * meshData.verticesPBD[i].invMass;
    }

    float c_p1p2p3 = curVolume - meshData.m_volume * m_volumePressure;

    float scaling = -c_p1p2p3 / gradSum;

    return scaling;
}

void BasicMeshGroup::solveOverpressureConstraint(
    MeshData &meshData, Triangle &t, float scaling) {
    int index0 = t.vertexIndices[0];
    int index1 = t.vertexIndices[1];
    int index2 = t.vertexIndices[2];

    float k = 1.0f;

    Vector3 pos1 = meshData.verticesPBD[index0].newPosition;
    Vector3 pos2 = meshData.verticesPBD[index1].newPosition;
    Vector3 pos3 = meshData.verticesPBD[index2].newPosition;

    float invMass1 = meshData.verticesPBD[index0].invMass;
    float invMass2 = meshData.verticesPBD[index1].invMass;
    float invMass3 = meshData.verticesPBD[index2].invMass;

    Vector3 dp1 = (pos2 - pos1).Cross(pos3 - pos1);
    Vector3 dp2 = (pos3 - pos2).Cross(pos1 - pos2);
    Vector3 dp3 = (pos1 - pos3).Cross(pos2 - pos3);

    dp1 *= invMass1 * scaling;
    dp2 *= invMass2 * scaling;
    dp3 *= invMass3 * scaling;

    pos1 += k * dp1;
    pos2 += k * dp2;
    pos3 += k * dp3;

    meshData.verticesPBD[index0].newPosition = pos1;
    meshData.verticesPBD[index1].newPosition = pos2;
    meshData.verticesPBD[index2].newPosition = pos3;

    // 누적 후, 한 번에 업데이트
    //meshData.verticesPBD[index0].gradPosition += k * dp1;
    //meshData.verticesPBD[index1].gradPosition += k * dp2;
    //meshData.verticesPBD[index2].gradPosition += k * dp3;
}

void BasicMeshGroup::Integrate(float dt) {
    for (auto &mesh : m_meshes) {
    MeshData &meshData = mesh->m_meshData;
    for (int i = 0; i < meshData.verticesPBD.size(); ++i) {
        meshData.verticesPBD[i].velocity =
            (meshData.verticesPBD[i].newPosition -
                meshData.vertices[i].position) /
            dt;
        meshData.vertices[i].position = meshData.verticesPBD[i].newPosition;
        }
    }
}

bool BasicMeshGroup::IntersectRayMesh(Ray &ray) {

    float closedHit = 10000.0f;
    bool collisionCheck = false;
    bool collisionTriangleCheck = false;

    for (auto &mesh : m_meshes) {
        for (auto& triangle : mesh->m_meshData.triangles)
        {
            Hit hit;
            collisionTriangleCheck =
                IntersectRayTriangle(mesh->m_meshData, ray, triangle, hit);

            if (collisionTriangleCheck && closedHit > hit.t)
            {
                collisionCheck = true;
                closedHit = hit.t;
                m_dragMeshData = &mesh->m_meshData;
                m_dragTriangle = triangle;
            }
        }
    }

    return collisionCheck;
}

bool BasicMeshGroup::IntersectRayTriangle(MeshData &meshData, Ray &ray,
                                        Triangle &triangle, Hit &hit) {
    int index0 = triangle.vertexIndices[0];
    int index1 = triangle.vertexIndices[1];
    int index2 = triangle.vertexIndices[2];

    Vector3 pos0 = meshData.vertices[index0].position;
    Vector3 pos1 = meshData.vertices[index1].position;
    Vector3 pos2 = meshData.vertices[index2].position;

    Matrix model = m_basicVertexConstantData.model.Transpose();
    // World space에서 충돌 계산
    pos0 = Vector3::Transform(pos0, model);
    pos1 = Vector3::Transform(pos1, model);
    pos2 = Vector3::Transform(pos2, model);

    Vector3 normal = (pos1 - pos0).Cross(pos2 - pos0);
    normal.Normalize();

    if (ray.rayDir.Dot(normal) > 0.0f)
        return false;

    // 평면과 광선이 수평에 매우 가깝다면 충돌하지 못하는 것으로 판단
    if (abs(ray.rayDir.Dot(normal)) < 1e-6f)
        return false; // t 계산시 0으로 나누기 방지

    float t =
        (pos0.Dot(normal) - ray.rayOrigin.Dot(normal)) / ray.rayDir.Dot(normal);

    // ray 뒤쪽
    if (t < 0)
        return false;

    hit.point = ray.rayOrigin + ray.rayDir * t;
    hit.normal = normal;
    hit.t = t;

    Vector3 cross0 = (pos0 - hit.point).Cross(pos1 - hit.point);
    Vector3 cross1 = (pos1 - hit.point).Cross(pos2 - hit.point);
    Vector3 cross2 = (pos2 - hit.point).Cross(pos0 - hit.point);

    // 삼각형 외부
    if (cross0.Dot(normal) < 0.0f || cross1.Dot(normal) < 0.0f ||
        cross2.Dot(normal) < 0.0f)
        return false;

    return true;
}

void BasicMeshGroup::MouseDrag(int x, int y, int screenWidth, int screenHeight)
{
    int index0 = m_dragTriangle.vertexIndices[0];
    int index1 = m_dragTriangle.vertexIndices[1];
    int index2 = m_dragTriangle.vertexIndices[2];

    // Interaction Vertices position(screen space) update
    Matrix model = m_basicVertexConstantData.model.Transpose();
    Matrix view = m_basicVertexConstantData.view.Transpose();
    Matrix projection = m_basicVertexConstantData.projection.Transpose();

    Vector3 prePoint = Vector3::Transform(
        Vector3::Transform(
            Vector3::Transform(m_dragMeshData->vertices[index0].position, model),
            view),
        projection);

    m_point.x = (prePoint.x + 1.0f) / 2.0f * screenWidth;
    m_point.y = -(prePoint.y - 1.0f) / 2.0f * screenHeight;

    // delta  = (Interaction Vertices position - mouse position)
    m_dragMeshData->vertices[index0].position +=
        (m_dragX * (x - m_point.x) - m_dragY * (y - m_point.y)) * 0.001f;
    m_dragMeshData->vertices[index1].position +=
        (m_dragX * (x - m_point.x) - m_dragY * (y - m_point.y)) * 0.001f;
    m_dragMeshData->vertices[index2].position +=
        (m_dragX * (x - m_point.x) - m_dragY * (y - m_point.y)) * 0.001f;

    }

void BasicMeshGroup::CheckCut(MeshData &meshData, Edge &e, Vector2 line) {
        Vector3 pos0 = meshData.vertices[e.index0].position;
        Vector3 pos1 = meshData.vertices[e.index1].position;

        // line은 방향벡터라고 가정 (원점을 지나는 직선)
        line.Normalize();

        Vector2 edgeDirection = Vector2(pos1) - Vector2(pos0);

        // denom = cross(line, edgeDirection)
        float denom = line.x * edgeDirection.y - line.y * edgeDirection.x;

        // c0 = cross(line, pos0), c1 = cross(line, pos1)
        float c0 = line.y * pos0.x - line.x * pos0.y;
        float c1 = line.y * pos1.x - line.x * pos1.y;

        const float eps = 1e-6f;

         if (true)
        //if (pos0.z < -1.5f && pos1.z < -1.5f)
        {
            // 1) 평행(denom ~ 0) 케이스
            if (fabs(denom) < eps) {
                // 1-a) 겹침(colinear): 두 점 모두 선 위
                if (fabs(c0) < eps && fabs(c1) < eps) {
                    Vertex addUpVertex;
                    VertexPBD addUpVertexPBD;
                    addUpVertex.position = pos0;
                    if (meshData.m_collisionVertices[e.index0] == -1) {
                        meshData.m_collisionVertices[e.index0] =
                            (int)meshData.vertices.size();
                        meshData.vertices.push_back(addUpVertex);
                        meshData.verticesPBD.push_back(addUpVertexPBD);
                        meshData.m_collisionVertices.push_back(-1);
                    }

                    Vertex addDownVertex;
                    VertexPBD addDownVertexPBD;
                    addDownVertex.position = pos1;
                    if (meshData.m_collisionVertices[e.index1] == -1) {
                        meshData.m_collisionVertices[e.index1] =
                            (int)meshData.vertices.size();
                        meshData.vertices.push_back(addDownVertex);
                        meshData.verticesPBD.push_back(addDownVertexPBD);
                        meshData.m_collisionVertices.push_back(-1);
                    }
                }

                // 1-b) 그냥 평행(겹치지 않음): 교차 없음
                return;
            }

            // 2) 교차 파라미터 t (0~1)
            //    pos = pos0 + t*(pos1-pos0)

            float t = c0 / denom;

            // 3) 점 동일 교차: t==0 또는 t==1
            if (fabs(t - 0.0f) < eps) {
                Vertex addVertex;
                VertexPBD addVertexPBD;
                addVertex.position = pos0;

                if (meshData.m_collisionVertices[e.index0] == -1) {
                    meshData.m_collisionVertices[e.index0] =
                        (int)meshData.vertices.size();
                    meshData.vertices.push_back(addVertex);
                    meshData.verticesPBD.push_back(addVertexPBD);
                    meshData.m_collisionVertices.push_back(-1);
                }
                return;
            } else if (fabs(t - 1.0f) < eps) {
                Vertex addVertex;
                VertexPBD addVertexPBD;
                addVertex.position = pos0;

                if (meshData.m_collisionVertices[e.index0] == -1) {
                    meshData.m_collisionVertices[e.index0] =
                        (int)meshData.vertices.size();
                    meshData.vertices.push_back(addVertex);
                    meshData.verticesPBD.push_back(addVertexPBD);
                    meshData.m_collisionVertices.push_back(-1);
                }
                return;
            }

            // 4) 사이 교차: 0 < t < 1
            if (t > 0.0f && t < 1.0f) {
                Vertex vertexUp, vertexDown;
                VertexPBD VertexUpPBD, VertexDownPBD;

                Vector3 cutPos = pos0 + t * (pos1 - pos0);

                vertexUp.position = cutPos;
                vertexDown.position = cutPos;

                e.cutVertexIndexUp = (int)meshData.vertices.size();
                meshData.vertices.push_back(vertexUp);
                meshData.verticesPBD.push_back(VertexUpPBD);
                meshData.m_collisionVertices.push_back(-1);

                e.cutVertexIndexDown = (int)meshData.vertices.size();
                meshData.vertices.push_back(vertexDown);
                meshData.verticesPBD.push_back(VertexDownPBD);
                meshData.m_collisionVertices.push_back(-1);

                return;
            }

            // 5) 나머지(선분 바깥 교차): 아무것도 안 함
            return;
        }
    }

void BasicMeshGroup::CutTwoEdge()
    {

}

void BasicMeshGroup::CutEdge(MeshData &meshData, Triangle &t,
                                Vector2 line) {
        Edge e0 = meshData.edges[t.edgeIndices[0]];
        Edge e1 = meshData.edges[t.edgeIndices[1]];
        Edge e2 = meshData.edges[t.edgeIndices[2]];

        Vector3 checkLine = Vector3(line.x, line.y, 0.0f);
        if (e0.cutVertexIndexDown != -1 && e1.cutVertexIndexDown != -1 &&
            e2.cutVertexIndexDown == -1) {
            Vector3 startVertexPos =
                meshData.vertices[t.vertexIndices[1]].position;
            Vector3 checkStartPos =
                Vector3(startVertexPos.x, startVertexPos.y, 0.0f);
            Vector3 checkPos =
                meshData.vertices[e0.cutVertexIndexDown].position;
            checkPos = Vector3(checkPos.x, checkPos.y, 0.0f);

            Vector3 check = (checkStartPos - checkPos).Cross(checkLine);
            if (check.z > 0.0f) {
                Triangle t0;
                t0.vertexIndices[0] = t.vertexIndices[0];
                t0.vertexIndices[1] = e0.cutVertexIndexDown;
                t0.vertexIndices[2] = e1.cutVertexIndexDown;

                Triangle t1;
                t1.vertexIndices[0] = t.vertexIndices[0];
                t1.vertexIndices[1] = e1.cutVertexIndexDown;
                t1.vertexIndices[2] = t.vertexIndices[2];

                t.vertexIndices[0] = e0.cutVertexIndexUp;
                t.vertexIndices[2] = e1.cutVertexIndexUp;

                meshData.triangles.push_back(t0);
                meshData.triangles.push_back(t1);

            } else {
                Triangle t0;
                t0.vertexIndices[0] = t.vertexIndices[0];
                t0.vertexIndices[1] = e0.cutVertexIndexUp;
                t0.vertexIndices[2] = e1.cutVertexIndexUp;

                Triangle t1;
                t1.vertexIndices[0] = t.vertexIndices[0];
                t1.vertexIndices[1] = e1.cutVertexIndexUp;
                t1.vertexIndices[2] = t.vertexIndices[2];

                t.vertexIndices[0] = e0.cutVertexIndexDown;
                t.vertexIndices[2] = e1.cutVertexIndexDown;

                meshData.triangles.push_back(t0);
                meshData.triangles.push_back(t1);
            }
        }

        else if (e0.cutVertexIndexDown == -1 && e1.cutVertexIndexDown != -1 &&
                 e2.cutVertexIndexDown != -1) {
            Vector3 startVertexPos =
                meshData.vertices[t.vertexIndices[2]].position;
            Vector3 checkStartPos =
                Vector3(startVertexPos.x, startVertexPos.y, 0.0f);
            Vector3 checkPos =
                meshData.vertices[e1.cutVertexIndexDown].position;
            checkPos = Vector3(checkPos.x, checkPos.y, 0.0f);

            Vector3 check = (checkStartPos - checkPos).Cross(checkLine);
            if (check.z > 0.0) {
                Triangle t0;
                t0.vertexIndices[0] = t.vertexIndices[1];
                t0.vertexIndices[1] = e1.cutVertexIndexDown;
                t0.vertexIndices[2] = e2.cutVertexIndexDown;

                Triangle t1;
                t1.vertexIndices[0] = t.vertexIndices[1];
                t1.vertexIndices[1] = e2.cutVertexIndexDown;
                t1.vertexIndices[2] = t.vertexIndices[0];

                t.vertexIndices[0] = e2.cutVertexIndexUp;
                t.vertexIndices[1] = e1.cutVertexIndexUp;

                meshData.triangles.push_back(t0);
                meshData.triangles.push_back(t1);

            } else {
                Triangle t0;
                t0.vertexIndices[0] = t.vertexIndices[1];
                t0.vertexIndices[1] = e1.cutVertexIndexUp;
                t0.vertexIndices[2] = e2.cutVertexIndexUp;

                Triangle t1;
                t1.vertexIndices[0] = t.vertexIndices[1];
                t1.vertexIndices[1] = e2.cutVertexIndexUp;
                t1.vertexIndices[2] = t.vertexIndices[0];

                t.vertexIndices[0] = e2.cutVertexIndexDown;
                t.vertexIndices[1] = e1.cutVertexIndexDown;

                meshData.triangles.push_back(t0);
                meshData.triangles.push_back(t1);
            }
        }

        else if (e0.cutVertexIndexDown != -1 && e1.cutVertexIndexDown == -1 &&
                 e2.cutVertexIndexDown != -1) {
            Vector3 startVertexPos =
                meshData.vertices[t.vertexIndices[0]].position;
            Vector3 checkStartPos =
                Vector3(startVertexPos.x, startVertexPos.y, 0.0f);
            Vector3 checkPos =
                meshData.vertices[e0.cutVertexIndexDown].position;
            checkPos = Vector3(checkPos.x, checkPos.y, 0.0f);

            Vector3 check = (checkStartPos - checkPos).Cross(checkLine);
            if (check.z > 0.0) {
                Triangle t0;
                t0.vertexIndices[0] = t.vertexIndices[2];
                t0.vertexIndices[1] = e2.cutVertexIndexDown;
                t0.vertexIndices[2] = e0.cutVertexIndexDown;

                Triangle t1;
                t1.vertexIndices[0] = t.vertexIndices[2];
                t1.vertexIndices[1] = e0.cutVertexIndexDown;
                t1.vertexIndices[2] = t.vertexIndices[1];

                t.vertexIndices[1] = e0.cutVertexIndexUp;
                t.vertexIndices[2] = e2.cutVertexIndexUp;

                meshData.triangles.push_back(t0);
                meshData.triangles.push_back(t1);

            } else {
                Triangle t0;
                t0.vertexIndices[0] = t.vertexIndices[2];
                t0.vertexIndices[1] = e2.cutVertexIndexUp;
                t0.vertexIndices[2] = e0.cutVertexIndexUp;

                Triangle t1;
                t1.vertexIndices[0] = t.vertexIndices[2];
                t1.vertexIndices[1] = e0.cutVertexIndexUp;
                t1.vertexIndices[2] = t.vertexIndices[1];

                t.vertexIndices[1] = e0.cutVertexIndexDown;
                t.vertexIndices[2] = e2.cutVertexIndexDown;

                meshData.triangles.push_back(t0);
                meshData.triangles.push_back(t1);
            }
        }

        else if (e0.cutVertexIndexDown != -1 && e1.cutVertexIndexDown == -1 &&
                 e2.cutVertexIndexDown == -1) {
            Vector3 startVertexPos =
                meshData.vertices[t.vertexIndices[1]].position;
            Vector3 checkStartPos =
                Vector3(startVertexPos.x, startVertexPos.y, 0.0f);
            Vector3 checkPos =
                meshData.vertices[e0.cutVertexIndexDown].position;
            checkPos = Vector3(checkPos.x, checkPos.y, 0.0f);

            Vector3 check = (checkStartPos - checkPos).Cross(checkLine);
            if (check.z > 0.0f) {
                Triangle t0;
                t0.vertexIndices[0] = e0.cutVertexIndexUp;
                t0.vertexIndices[1] = t.vertexIndices[1];
                t0.vertexIndices[2] = t.vertexIndices[2];

                t.vertexIndices[1] = e0.cutVertexIndexDown;

                meshData.triangles.push_back(t0);
            } else {
                Triangle t0;
                t0.vertexIndices[0] = e0.cutVertexIndexDown;
                t0.vertexIndices[1] = t.vertexIndices[1];
                t0.vertexIndices[2] = t.vertexIndices[2];

                t.vertexIndices[1] = e0.cutVertexIndexUp;

                meshData.triangles.push_back(t0);
            }
        } else if (e0.cutVertexIndexDown == -1 && e1.cutVertexIndexDown != -1 &&
                   e2.cutVertexIndexDown == -1) {
            Vector3 startVertexPos =
                meshData.vertices[t.vertexIndices[1]].position;
            Vector3 checkStartPos =
                Vector3(startVertexPos.x, startVertexPos.y, 0.0f);
            Vector3 checkPos =
                meshData.vertices[e1.cutVertexIndexDown].position;
            checkPos = Vector3(checkPos.x, checkPos.y, 0.0f);

            Vector3 check = (checkStartPos - checkPos).Cross(checkLine);
            if (check.z > 0.0f) {
                Triangle t0;
                t0.vertexIndices[0] = e1.cutVertexIndexUp;
                t0.vertexIndices[1] = t.vertexIndices[0];
                t0.vertexIndices[2] = t.vertexIndices[1];

                t.vertexIndices[1] = e1.cutVertexIndexDown;

                meshData.triangles.push_back(t0);
            } else {
                Triangle t0;
                t0.vertexIndices[0] = e1.cutVertexIndexDown;
                t0.vertexIndices[1] = t.vertexIndices[0];
                t0.vertexIndices[2] = t.vertexIndices[1];

                t.vertexIndices[1] = e1.cutVertexIndexUp;

                meshData.triangles.push_back(t0);
            }
        } else if (e0.cutVertexIndexDown == -1 && e1.cutVertexIndexDown == -1 &&
                   e2.cutVertexIndexDown != -1) {
            Vector3 startVertexPos =
                meshData.vertices[t.vertexIndices[2]].position;
            Vector3 checkStartPos =
                Vector3(startVertexPos.x, startVertexPos.y, 0.0f);
            Vector3 checkPos =
                meshData.vertices[e2.cutVertexIndexDown].position;
            checkPos = Vector3(checkPos.x, checkPos.y, 0.0f);

            Vector3 check = (checkStartPos - checkPos).Cross(checkLine);
            if (check.z > 0.0f) {
                Triangle t0;
                t0.vertexIndices[0] = e2.cutVertexIndexUp;
                t0.vertexIndices[1] = t.vertexIndices[1];
                t0.vertexIndices[2] = t.vertexIndices[2];

                t.vertexIndices[2] = e2.cutVertexIndexDown;

                meshData.triangles.push_back(t0);
            } else {
                Triangle t0;
                t0.vertexIndices[0] = e2.cutVertexIndexDown;
                t0.vertexIndices[1] = t.vertexIndices[1];
                t0.vertexIndices[2] = t.vertexIndices[2];

                t.vertexIndices[2] = e2.cutVertexIndexUp;

                meshData.triangles.push_back(t0);
            }
        }
    };

void BasicMeshGroup::CutVertex(MeshData &meshData, Triangle &t,
                                   Vector2 line) {
        const float eps = 1e-6f;

        Vector3 pos0 = meshData.vertices[t.vertexIndices[0]].position;
        Vector3 pos1 = meshData.vertices[t.vertexIndices[1]].position;
        Vector3 pos2 = meshData.vertices[t.vertexIndices[2]].position;

        int idx0 = meshData.m_collisionVertices[t.vertexIndices[0]];
        int idx1 = meshData.m_collisionVertices[t.vertexIndices[1]];
        int idx2 = meshData.m_collisionVertices[t.vertexIndices[2]];

        Vector3 checkLine = Vector3(line.x, line.y, 0.0f);

        if (idx0 != -1 && idx1 != -1 && idx2 == -1) {
            Vector3 checkPos = pos2 - pos1;
            checkPos = Vector3(checkPos.x, checkPos.y, 0.0f);

            Vector3 check = checkPos.Cross(checkLine);
            if (check.z > 0.0f) {
                t.vertexIndices[0] = idx0;
                t.vertexIndices[1] = idx1;
            }
        } else if (idx0 != -1 && idx1 == -1 && idx2 != -1) {
            Vector3 checkPos = pos1 - pos0;
            checkPos = Vector3(checkPos.x, checkPos.y, 0.0f);

            Vector3 check = checkPos.Cross(checkLine);
            if (check.z > 0.0f) {
                t.vertexIndices[0] = idx0;
                t.vertexIndices[2] = idx2;
            }
        } else if (idx0 == -1 && idx1 != -1 && idx2 != -1) {
            Vector3 checkPos = pos0 - pos1;
            checkPos = Vector3(checkPos.x, checkPos.y, 0.0f);

            Vector3 check = checkPos.Cross(checkLine);
            if (check.z > 0.0f) {
                t.vertexIndices[1] = idx1;
                t.vertexIndices[2] = idx2;
            }
        } else if (idx0 != -1 && idx1 == -1 && idx2 == -1) {
            Vector3 checkPosUp = pos1 - pos0;
            checkPosUp = Vector3(checkPosUp.x, checkPosUp.y, 0.0f);

            Vector3 checkUp = checkPosUp.Cross(checkLine);

            Vector3 checkPosDown = pos2 - pos0;
            checkPosDown = Vector3(checkPosDown.x, checkPosDown.y, 0.0f);

            Vector3 checkDown = checkPosDown.Cross(checkLine);

            if (checkUp.z > eps)
                t.vertexIndices[0] = idx0;

            else if (checkDown.z > eps)
                t.vertexIndices[0] = idx0;
        } else if (idx0 == -1 && idx1 != -1 && idx2 == -1) {
            Vector3 checkPosUp = pos0 - pos1;
            checkPosUp = Vector3(checkPosUp.x, checkPosUp.y, 0.0f);

            Vector3 checkUp = checkPosUp.Cross(checkLine);

            Vector3 checkPosDown = pos2 - pos1;
            checkPosDown = Vector3(checkPosDown.x, checkPosDown.y, 0.0f);

            Vector3 checkDown = checkPosDown.Cross(checkLine);

            if (checkUp.z > eps)
                t.vertexIndices[1] = idx1;

            else if (checkDown.z > eps)
                t.vertexIndices[1] = idx1;
        } else if (idx0 == -1 && idx1 == -1 && idx2 != -1) {
            Vector3 checkPosUp = pos0 - pos2;
            checkPosUp = Vector3(checkPosUp.x, checkPosUp.y, 0.0f);

            Vector3 checkUp = checkPosUp.Cross(checkLine);

            Vector3 checkPosDown = pos1 - pos2;
            checkPosDown = Vector3(checkPosDown.x, checkPosDown.y, 0.0f);

            Vector3 checkDown = checkPosDown.Cross(checkLine);

            if (checkUp.z > eps)
                t.vertexIndices[2] = idx2;

            else if (checkDown.z > eps)
                t.vertexIndices[2] = idx2;
        }
    }

int BasicMeshGroup::AddEdge(MeshData& meshData, int index0, int index1)
{
        bool check = false;
        int idx = 0;
        int idx0 = std::min(index0, index1);
        int idx1 = std::max(index0, index1);

        for (auto &e : meshData.edges) {
            if (e.index0 == idx0 && e.index1 == idx1) {

                check = true;
                break;
            }
            ++idx;
        }

        if (check == false) {
            Edge add;
            add.index0 = idx0;
            add.index1 = idx1;
            add.restLength = (meshData.vertices[idx0].position -
                              meshData.vertices[idx1].position)
                                 .Length();
            meshData.edges.push_back(add);
        }

        return idx;
    }

void BasicMeshGroup::LineCut(Vector2 line) {
        for (auto &mesh : m_meshes) {
            MeshData &meshData = mesh->m_meshData;
            meshData.vertices.resize(meshData.verticesPBD.size());  // vertices에 있는 기존 particles 삭제
            meshData.m_collisionVertices.assign(meshData.vertices.size(), -1);

            for (auto &e : meshData.edges) {
                CheckCut(meshData, e, line);
            }

            int initTrianglesSize = meshData.triangles.size();

            for (int i = 0; i < initTrianglesSize; ++i) {
                CutEdge(meshData, meshData.triangles[i], line);
            }

            initTrianglesSize = meshData.triangles.size();

            for (int i = 0; i < initTrianglesSize; ++i) {
                CutVertex(meshData, meshData.triangles[i], line);
            }

            meshData.indices.clear();
            meshData.edges.clear();
            for (auto &t : meshData.triangles) {
                int vertexIndex0 = t.vertexIndices[0];
                int vertexIndex1 = t.vertexIndices[1];
                int vertexIndex2 = t.vertexIndices[2];

                meshData.indices.push_back(vertexIndex0);
                meshData.indices.push_back(vertexIndex1);
                meshData.indices.push_back(vertexIndex2);

                t.edgeIndices[0] = 
                    AddEdge(meshData, vertexIndex0, vertexIndex1);
                t.edgeIndices[1] =
                    AddEdge(meshData, vertexIndex1, vertexIndex2);
                t.edgeIndices[2] =
                    AddEdge(meshData, vertexIndex2, vertexIndex0);
            }
            UpdateNormal();
            InitParticles();
        }
    }

void BasicMeshGroup::UpdateNormal()
{
        for (auto &mesh : m_meshes) {
        vector<int> countNormal(mesh->m_meshData.vertices.size(), 0);

            for (int i = 0; i < mesh->m_meshData.vertices.size(); ++i) {
                mesh->m_meshData.vertices[i].normal = Vector3(0.0f, 0.0f, 0.0f);
            }

            for (auto& triangle : mesh->m_meshData.triangles) {
                int idx1 = triangle.vertexIndices[0];
                int idx2 = triangle.vertexIndices[1];
                int idx3 = triangle.vertexIndices[2];

                Vector3 pos1 = mesh->m_meshData.vertices[idx1].position;
                Vector3 pos2 = mesh->m_meshData.vertices[idx2].position;
                Vector3 pos3 = mesh->m_meshData.vertices[idx3].position;

                Vector3 normal = (pos2 - pos1).Cross(pos3 - pos1);
                normal.Normalize();

                mesh->m_meshData.vertices[idx1].normal += normal;
                mesh->m_meshData.vertices[idx2].normal += normal;
                mesh->m_meshData.vertices[idx3].normal += normal;

                countNormal[idx1] += 1;
                countNormal[idx2] += 1;
                countNormal[idx3] += 1;
            }

            for (int i = 0; i < mesh->m_meshData.vertices.size(); ++i) {
                mesh->m_meshData.vertices[i].normal /= countNormal[i];
                mesh->m_meshData.vertices[i].normal.Normalize();
            }
        }
    }

void BasicMeshGroup::UpdateNormalLines(ComPtr<ID3D11Device> &device,
                                      ComPtr<ID3D11DeviceContext> &context) {

        std::vector<Vertex> normalVertices;
        std::vector<uint32_t> normalIndices;

        // 여러 메쉬의 normal 들을 하나로 합치기
        size_t offset = 0;
        for (const auto &mesh : m_meshes) {
            for (size_t i = 0; i < mesh->m_meshData.vertices.size(); i++) {

                auto v = mesh->m_meshData.vertices[i];

                v.texcoord.x = 0.0f; // 시작점 표시
                normalVertices.push_back(v);

                v.texcoord.x = 1.0f; // 끝점 표시
                normalVertices.push_back(v);

                normalIndices.push_back(uint32_t(2 * (i + offset)));
                normalIndices.push_back(uint32_t(2 * (i + offset) + 1));
            }
            offset += mesh->m_meshData.vertices.size();
        }

        UpdateNormalLinesVertexBuffers(device, context, normalVertices,
                                    m_normalLines->vertexBuffer);
        UpdateNormalLinesIndexBuffers(device, context, normalIndices,
                                    m_normalLines->indexBuffer);
        m_normalLines->m_indexCount = normalIndices.size();
        
    }

void BasicMeshGroup::PrintParticleCount() {
        std::cout << "Particle Count: " << m_meshes[0]->m_meshData.vertices.size() << std::endl;
    }
} // namespace jhm