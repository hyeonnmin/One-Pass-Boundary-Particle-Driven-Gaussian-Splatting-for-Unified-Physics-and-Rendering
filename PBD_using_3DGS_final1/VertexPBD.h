#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>

namespace jhm {

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;

struct VertexPBD {
    Vector3 newPosition = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 gradPosition;
    Vector3 velocity = Vector3(0.0f, 0.0f, 0.0f);
    float invMass = 1.0f;
    float bendingForce;
    int countNormal = 0;
};

} // namespace hlab