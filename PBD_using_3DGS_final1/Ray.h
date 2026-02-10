#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>

using DirectX::SimpleMath::Vector3;

namespace jhm {

struct Ray {
    Vector3 rayOrigin;
    Vector3 rayDir;
};
} // namespace hlab