#pragma once

#include "BasicConstantData.h"
#include "Mesh.h"
#include "MeshData.h"
#include "Ray.h"
#include "Hit.h"
#include "D3D11Utils.h"

namespace jhm {

class BasicMeshGroup {
  public:
    void Initialize(ComPtr<ID3D11Device> &device, const std::string &basePath,
                    const std::string &filename);

    void Initialize(ComPtr<ID3D11Device> &device,
                    const std::vector<MeshData> &meshes);

    void UpdateConstantBuffers(ComPtr<ID3D11Device> &device,
                               ComPtr<ID3D11DeviceContext> &context);

    void UpdateVertexBuffers(ComPtr<ID3D11Device> &device,
                        ComPtr<ID3D11DeviceContext> &context);
    void UpdateIndexBuffers(ComPtr<ID3D11Device> &device,
                             ComPtr<ID3D11DeviceContext> &context);
    template <typename T_DATA>
    void UpdateNormalLinesVertexBuffers(ComPtr<ID3D11Device> &device,
                                        ComPtr<ID3D11DeviceContext> &context,
                                        const T_DATA &bufferData,
                                        ComPtr<ID3D11Buffer> &buffer);
    template <typename T_DATA>
    void UpdateNormalLinesIndexBuffers(ComPtr<ID3D11Device> &device,
                                        ComPtr<ID3D11DeviceContext> &context,
                                        const T_DATA &bufferData,
                                        ComPtr<ID3D11Buffer> &buffer);
    void Render(ComPtr<ID3D11DeviceContext> &context);;

    // PBD Simulation 함수
    void InitParticles();
    void UpdateParticles();
    void EdgeSampling(MeshData &meshData, Edge &e, float d);
    void InnerSampling(MeshData &meshData, Triangle &t, float d);
    
    void ApplyExtForces(float dt);
    void ProjectDistanceConstraints(); 
    void ProjectDistanceConstraint(MeshData &meshData, Edge &e);
    void SolveOverpressureConstraints();
    float computeVolumeConstraintScaling(MeshData &meshData);
    void solveOverpressureConstraint(MeshData &meshData, Triangle &t, float scaling);
    void Integrate(float dt);
    
    void UpdateNormal();
    void UpdateNormalLines(ComPtr<ID3D11Device> &device,
                           ComPtr<ID3D11DeviceContext> &context);
    // Mouse 
    bool IntersectRayMesh(Ray &ray);
    bool IntersectRayTriangle(MeshData &meshData, Ray &ray, Triangle &triangle,
                              Hit &hit);
    void MouseDrag(int x, int y, int screenWidth, int screenHeight);

    // Tearing
    void CheckCut(MeshData &meshData, Edge &e, Vector2 line);
    void CutEdge(MeshData &meshData, Triangle &t, Vector2 line);
    void CutTwoEdge();
    void CutVertex(MeshData &meshData, Triangle &t, Vector2 line);
    int AddEdge(MeshData &meshData, int index0, int index1);
    void LineCut(Vector2 line);

    // Print Particle Count
    void PrintParticleCount();
  public:
    // ExampleApp::Update()에서 접근
    BasicVertexConstantData m_basicVertexConstantData;
    BasicGeometryConstantData m_basicGeometryConstantData;
    BasicPixelConstantData m_basicPixelConstantData;

    // ExampleApp:Initialize()에서 접근
    ComPtr<ID3D11ShaderResourceView> m_diffuseResView;
    ComPtr<ID3D11ShaderResourceView> m_specularResView;

    // GUI에서 업데이트 할 때 사용
    NormalVertexConstantData m_normalVertexConstantData;
    bool m_drawNormalsDirtyFlag = true;
    bool m_drawNormals = false;

    // Volume
    float m_volumePressure = 1.0f;

    // Particle 거리 
    float m_particle_distance = 0.04f;

    // Gaussian scale
    float m_gaussian_scaling = 0.76f;

    // Mouse
    MeshData* m_dragMeshData = nullptr;
    Triangle m_dragTriangle;
    Vector3 m_dragX;
    Vector3 m_dragY;
    Vector2 m_point;

    // tearing
    bool m_LineCollision = false;

    bool m_useTexture = false;
  private:
    // 메쉬 그리기
    std::vector<shared_ptr<Mesh>> m_meshes;

    ComPtr<ID3D11VertexShader> m_basicVertexShader;
    ComPtr<ID3D11GeometryShader> m_basicGeometryShader;
    ComPtr<ID3D11PixelShader> m_basicPixelShader;
    ComPtr<ID3D11InputLayout> m_basicInputLayout;
    ComPtr<ID3D11InputLayout> m_GSInputLayout;

    ComPtr<ID3D11SamplerState> m_samplerState;

    ComPtr<ID3D11Buffer> m_vertexConstantBuffer;
    ComPtr<ID3D11Buffer> m_geometryConstantBuffer;
    ComPtr<ID3D11Buffer> m_pixelConstantBuffer;

    // 메쉬의 노멀 벡터 그리기
    ComPtr<ID3D11VertexShader> m_normalVertexShader;
    ComPtr<ID3D11PixelShader> m_normalPixelShader;

    shared_ptr<Mesh> m_normalLines;

    ComPtr<ID3D11Buffer> m_normalVertexConstantBuffer;
    ComPtr<ID3D11Buffer> m_normalPixelConstantBuffer;
};

} // namespace hlab
