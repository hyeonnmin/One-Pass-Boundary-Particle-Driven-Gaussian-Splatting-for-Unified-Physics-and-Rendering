#include "ExampleApp.h"

#include <directxtk/DDSTextureLoader.h> // 큐브맵 읽을 때 필요
#include <tuple>
#include <vector>

#include "GeometryGenerator.h"

namespace jhm {

using namespace std;
using namespace DirectX;

ExampleApp::ExampleApp() : AppBase() {}

bool ExampleApp::Initialize() {

    if (!AppBase::Initialize())
        return false;

    m_cubeMapping.Initialize(m_device,
                             L"./CubemapTextures/Stonewall_diffuseIBL.dds",
                             L"./CubemapTextures/Stonewall_specularIBL.dds");

    MeshData sphere = GeometryGenerator::MakeSphere(0.5f, 30, 30);
    sphere.textureFilename = "ojwD8.jpg";
    m_meshGroupSphere.Initialize(m_device, {sphere});
    m_meshGroupSphere.InitParticles();
    m_meshGroupSphere.m_diffuseResView = m_cubeMapping.m_diffuseResView;
    m_meshGroupSphere.m_specularResView = m_cubeMapping.m_specularResView;
    m_meshGroup.push_back(&m_meshGroupSphere);

    m_meshGroupObject.Initialize(m_device, "C:/Users/wjdgu/Desktop/OBJ/",
                                    "dragon.obj");
    m_meshGroupObject.InitParticles();
    m_meshGroupObject.m_diffuseResView = m_cubeMapping.m_diffuseResView;
    m_meshGroupObject.m_specularResView = m_cubeMapping.m_specularResView;
    m_meshGroup.push_back(&m_meshGroupObject);

    m_meshGroupCharacter.Initialize(m_device, "C:/Users/wjdgu/Desktop/", "fandisk.obj");
    m_meshGroupCharacter.InitParticles();
    m_meshGroupCharacter.m_diffuseResView = m_cubeMapping.m_diffuseResView;
    m_meshGroupCharacter.m_specularResView = m_cubeMapping.m_specularResView;
    m_meshGroup.push_back(&m_meshGroupCharacter);

    //BuildFilters();

    return true;
}

void ExampleApp::Update(float dt) {

    using namespace DirectX;

    auto &visibleMeshGroup = *m_meshGroup[m_visibleMeshIndex];

    // Mouse Drag
    if (m_leftButtonDown) {
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(m_mainWindow, &p);

        if (m_collision)
            visibleMeshGroup.MouseDrag(p.x, p.y, m_screenWidth, m_screenHeight);
    }

    // PBD Simulation Update
    visibleMeshGroup.ApplyExtForces(dt);

    for (int i = 0; i < 5; ++i)
    {
        visibleMeshGroup.ProjectDistanceConstraints();
        visibleMeshGroup.SolveOverpressureConstraints();
    }

    visibleMeshGroup.Integrate(dt);
    visibleMeshGroup.UpdateNormal();
    visibleMeshGroup.UpdateParticles();

    // NormalLines & Buffer Update
    visibleMeshGroup.UpdateNormalLines(m_device, m_context);
    // Vertext Buffer Update
    visibleMeshGroup.UpdateVertexBuffers(m_device, m_context);
    visibleMeshGroup.UpdateIndexBuffers(m_device, m_context);

    // Constant Update
    auto modelRow = Matrix::CreateScale(m_modelScaling) *
                    Matrix::CreateRotationY(m_modelRotation.y) *
                    Matrix::CreateRotationX(m_modelRotation.x) *
                    Matrix::CreateRotationZ(m_modelRotation.z) *
                    Matrix::CreateTranslation(m_modelTranslation);

    auto invTransposeRow = modelRow;
    invTransposeRow.Translation(Vector3(0.0f));
    invTransposeRow = invTransposeRow.Invert().Transpose();

    auto viewRow = Matrix::CreateRotationY(m_viewRot.y) *
                   Matrix::CreateRotationX(m_viewRot.x) *
                   Matrix::CreateTranslation(m_viewTranslation);
    m_invView = viewRow.Invert();


    const float aspect = AppBase::GetAspectRatio();
    Matrix projRow =
        m_usePerspectiveProjection
            ? XMMatrixPerspectiveFovLH(XMConvertToRadians(m_projFovAngleY),
                                       aspect, m_nearZ, m_farZ)
            : XMMatrixOrthographicOffCenterLH(-aspect, aspect, -1.0f, 1.0f,
                                              m_nearZ, m_farZ);
    m_invProjection = projRow.Invert();

    auto eyeWorld = Vector3::Transform(Vector3(0.0f), viewRow.Invert());

    // MeshGroup의 ConstantBuffers 업데이트

    for (int i = 0; i < MAX_LIGHTS; i++) {
        // 다른 조명 끄기
        if (i != m_lightType) {
            visibleMeshGroup.m_basicPixelConstantData.lights[i].strength *=
                0.0f;
        } else {
            visibleMeshGroup.m_basicPixelConstantData.lights[i] =
                m_lightFromGUI;
        }
    }

    visibleMeshGroup.m_basicVertexConstantData.model = modelRow.Transpose();
    visibleMeshGroup.m_basicVertexConstantData.view = viewRow.Transpose();
    visibleMeshGroup.m_basicVertexConstantData.projection = projRow.Transpose();
    visibleMeshGroup.m_basicVertexConstantData.invTranspose = invTransposeRow.Transpose();

    visibleMeshGroup.m_basicGeometryConstantData.model = modelRow.Transpose();
    visibleMeshGroup.m_basicGeometryConstantData.view = viewRow.Transpose();
    visibleMeshGroup.m_basicGeometryConstantData.projection = projRow.Transpose();
    visibleMeshGroup.m_basicGeometryConstantData.invTranspose = invTransposeRow.Transpose();
    visibleMeshGroup.m_basicGeometryConstantData.invModel = modelRow.Invert().Transpose();
    visibleMeshGroup.m_basicGeometryConstantData.invView = viewRow.Invert().Transpose();
    visibleMeshGroup.m_basicGeometryConstantData.scaling = visibleMeshGroup.m_gaussian_scaling;

    visibleMeshGroup.m_basicPixelConstantData.eyeWorld = eyeWorld;

    visibleMeshGroup.m_basicPixelConstantData.material.diffuse =
        Vector3(m_materialDiffuse);
    visibleMeshGroup.m_basicPixelConstantData.material.specular =
        Vector3(m_materialSpecular);
    visibleMeshGroup.m_basicPixelConstantData.useTexture =
        visibleMeshGroup.m_useTexture;
    visibleMeshGroup.UpdateConstantBuffers(m_device, m_context);

    // 큐브 매핑 Constant Buffer 업데이트
    m_cubeMapping.UpdateConstantBuffers(m_device, m_context,
                                        (Matrix::CreateRotationY(m_viewRot.y) *
                                         Matrix::CreateRotationX(m_viewRot.x))
                                            .Transpose(),
                                        projRow.Transpose());

    static float time = 0.0f;

    //m_filters.back()->m_pixelConstData.iTime = time;
    //m_filters.back()->UpdateConstantBuffers(m_device, m_context);

    time += dt;
}

void ExampleApp::Render() {

    // GPU 시간 측정
    const int idx = int(m_frameId % GPU_QUERY_LATENCY);

    m_context->Begin(m_gpuTimer[idx].disjoint.Get());
    m_context->End(m_gpuTimer[idx].tsBegin.Get());

    // RS: Rasterizer stage
    // OM: Output-Merger stage
    // VS: Vertex Shader
    // PS: Pixel Shader
    // IA: Input-Assembler stage

    SetViewport();

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(),
                                     D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                     1.0f, 0);
    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(),
                                  m_depthStencilView.Get());
    m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

    if (m_drawAsWire) {
        m_context->RSSetState(m_wireRasterizerSate.Get());
    } else {
        m_context->RSSetState(m_rasterizerSate.Get());
    }

    // 큐브매핑
    m_cubeMapping.Render(m_context);

    // 물체들
    m_meshGroup[m_visibleMeshIndex]->Render(m_context);

   
    // 후처리 필터
    //for (auto &f : m_filters) {
    //    f->Render(m_context);
    //}

    m_context->End(m_gpuTimer[idx].tsEnd.Get());
    m_context->End(m_gpuTimer[idx].disjoint.Get());
    
    // 과거 프레임 결과 읽기
    const int readIdx =
        int((m_frameId + GPU_QUERY_LATENCY - 1) % GPU_QUERY_LATENCY);
    ReadBackGpuTimer(readIdx);

    ++m_frameId;
}

void ExampleApp::BuildFilters() {

    static ComPtr<ID3D11Texture2D> texture;
    static ComPtr<ID3D11ShaderResourceView> textureResourceView;

    D3D11Utils::CreateTexture(m_device, "shadertoytexture0.jpg", texture,
                              textureResourceView);

    m_filters.clear();

    //auto shaderToy =
    //    make_shared<ImageFilter>(m_device, m_context, L"Sampling", L"Seascape",
    //                             m_screenWidth, m_screenHeight);

    auto shaderToy =
        make_shared<ImageFilter>(m_device, m_context, L"Sampling", L"Star",
                                 m_screenWidth, m_screenHeight);


    shaderToy->SetShaderResources({textureResourceView});
    shaderToy->SetRenderTargets({m_renderTargetView});
    m_filters.push_back(shaderToy);
}

Ray ExampleApp::TransformScreenToWorld(int x, int y) {
    float ndcX = (x * (2.0f / this->m_screenWidth)) - 1.0f;
    float ndcY = -(y * (2.0f / this->m_screenHeight)) + 1.0f;

    Vector4 pNear(ndcX, ndcY, 0.0f, 1.0f); // D3D near z=0
    Vector4 pFar(ndcX, ndcY, 1.0f, 1.0f);  // D3D far  z=1

    Matrix invViewProj = m_invProjection * m_invView;
    Vector4 wNear = Vector4::Transform(pNear, invViewProj);
    Vector4 wFar = Vector4::Transform(pFar, invViewProj);

    wNear /= wNear.w;
    wFar /= wFar.w;

    Ray ray;
    Vector3 rayOrigin = (Vector3)wNear;
    Vector3 rayDir = (Vector3)wFar - (Vector3)wNear;
    rayDir.Normalize();
    ray.rayOrigin = rayOrigin;
    ray.rayDir = rayDir;

    return ray;
}

void ExampleApp::OnMouseDown(WPARAM btnState, int x, int y) {

    auto &visibleMeshGroup = m_meshGroup[m_visibleMeshIndex];

    m_leftButtonDown = true;
    
    Ray ray = TransformScreenToWorld(x, y);

    m_collision = visibleMeshGroup->IntersectRayMesh(ray);
    
    if (m_collision)
    {
        //cout << "collision" << endl;
        Matrix invModel = Matrix::CreateRotationY(m_modelRotation.y) *
                          Matrix::CreateRotationZ(m_modelRotation.z) *
                          Matrix::CreateRotationX(m_modelRotation.x);
        invModel = invModel.Invert();

        Matrix invView = Matrix::CreateRotationY(m_viewRot.y) *
                         Matrix::CreateRotationZ(m_viewRot.z) *
                         Matrix::CreateRotationX(m_viewRot.x);
        invView = invView.Invert();

        // Camera right/up 축 -> Model space 변환
        visibleMeshGroup->m_dragX = Vector3::Transform(
            Vector3::Transform(Vector3(1.0f, 0.0f, 0.0f), invView), invModel);
        visibleMeshGroup->m_dragY = Vector3::Transform(
            Vector3::Transform(Vector3(0.0f, 1.0f, 0.0f), invView), invModel);

        visibleMeshGroup->m_point = Vector2(x, y);
    }
}

void ExampleApp::OnMouseUp(WPARAM btnState, int x, int y) {
    m_leftButtonDown = false;
    m_collision = false;
}

void ExampleApp::LineCut(Vector2 line)
{
    auto &visibleMeshGroup = m_meshGroup[m_visibleMeshIndex];

    visibleMeshGroup->LineCut(line);

    visibleMeshGroup->m_LineCollision = false;
}

void ExampleApp::UpdateGUI() {
    if (ImGui::Button("Change Mesh Model")) {
        m_visibleMeshIndex += 1;
        m_visibleMeshIndex %= 3;
    }
    if (ImGui::Button("Line Cutting")) {
        m_meshGroup[m_visibleMeshIndex]->m_LineCollision = true;
        Vector2 lineX = Vector2(1.0f, 0.0f);
        Vector2 lineY = Vector2(0.0f, 1.0f);
        Vector2 lineXY = Vector2(1.0f, 1.0f);
        Vector2 lineYX = Vector2(1.0f, -1.0f);
        LineCut(lineXY);
        LineCut(lineYX);
    }

    if (ImGui::Button("Print Particle Count"))
    {
       m_meshGroup[m_visibleMeshIndex]->PrintParticleCount();
    }
    ImGui::SliderFloat("Particle Distance",
                       &m_meshGroup[m_visibleMeshIndex]->m_particle_distance,
                       0.0f, 0.1f);

    ImGui::SliderFloat("Gaussian Scale",
                       &m_meshGroup[m_visibleMeshIndex]->m_gaussian_scaling,
                       0.0f, 5.0f);

    ImGui::SliderFloat("m_modelVolume", &m_meshGroup[m_visibleMeshIndex]->m_volumePressure,
                        0.1f, 2.0f);

    ImGui::Checkbox("Draw Noraml", &m_meshGroup[m_visibleMeshIndex]->m_drawNormals);
    ImGui::Checkbox("Wireframe", &m_drawAsWire);
    ImGui::Checkbox("Use Texture", &m_meshGroup[m_visibleMeshIndex]->m_useTexture);
    ImGui::SliderFloat3("m_modelTranslation", &m_modelTranslation.x, -8.0f,
                        8.0f);
    ImGui::SliderFloat3("m_modelRotation", &m_modelRotation.x, -3.14f, 3.14f);
    ImGui::SliderFloat3("m_modelScaling", &m_modelScaling.x, 0.1f, 2.0f);
    ImGui::SliderFloat3("m_viewTranslation", &m_viewTranslation.x, -8.0f, 8.0f);
    ImGui::SliderFloat3("m_viewRot", &m_viewRot.x, -3.14f, 3.14f);

    ImGui::SliderFloat("Material Diffuse", &m_materialDiffuse, 0.0f, 1.0f);
    ImGui::SliderFloat("Material Specular", &m_materialSpecular, 0.0f, 1.0f);
}
} // namespace hlab
