#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "AppBase.h"
#include "CubeMapping.h"
#include "GeometryGenerator.h"
#include "Light.h"
#include "BasicMeshGroup.h"
#include "ImageFilter.h"

namespace jhm {

using DirectX::SimpleMath::Vector3;

class ExampleApp : public AppBase {
  public:
    ExampleApp();

    virtual bool Initialize() override;
    virtual void UpdateGUI() override;
    virtual void Update(float dt) override;
    virtual void Render() override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
    virtual Ray TransformScreenToWorld(int x, int y) override;

    virtual void LineCut(Vector2 line);

    void BuildFilters();

  protected:
    BasicMeshGroup m_meshGroupSphere;
    BasicMeshGroup m_meshGroupObject;
    BasicMeshGroup m_meshGroupCharacter;
    vector<BasicMeshGroup *> m_meshGroup;
    CubeMapping m_cubeMapping;

    bool m_usePerspectiveProjection = true;
    Vector3 m_modelTranslation = Vector3(0.0f);
    Vector3 m_modelRotation = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_modelScaling = Vector3(1.0f);
    Vector3 m_viewTranslation = Vector3(0.0f, 0.0f, 2.0f);
    Vector3 m_viewRot = Vector3(-0.1f, 0.0f, 0.0f);

    Matrix m_invView;
    Matrix m_invProjection;

    float m_projFovAngleY = 70.0f;
    float m_nearZ = 0.01f;
    float m_farZ = 100.0f;

    int m_lightType = 0;
    Light m_lightFromGUI;
    float m_materialDiffuse = 1.0f;
    float m_materialSpecular = 1.0f;

    int m_visibleMeshIndex = 0; // Sphere, character.

    std::vector<shared_ptr<ImageFilter>> m_filters;

    int m_dirtyflag = 1; // 처음에 한 번 실행
    int m_down = 16;
    int m_repeat = 5;
    float m_threshold = 0.3f;
    float m_strength = 1.0f;
};
} // namespace hlab