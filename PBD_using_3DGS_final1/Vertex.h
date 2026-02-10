#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>

static const int MAX_SH_COEFF = 48;

namespace jhm {

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;

struct Vertex {
    Vector3 position;
    Vector3 scale = {0.05f, 0.05f, 0.05f};
    Vector4 rot = {1, 0, 0, 0};
    float sh[MAX_SH_COEFF];
    float opacity;
    Vector3 normal;
    Vector2 texcoord;
    int countNormal = 0;
};

} // namespace jhm