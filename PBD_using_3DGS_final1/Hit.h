#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>

using DirectX::SimpleMath::Vector3;

namespace jhm {

struct Hit {
    Vector3 point;
    Vector3 normal;
    float t;
};
} // namespace hlab