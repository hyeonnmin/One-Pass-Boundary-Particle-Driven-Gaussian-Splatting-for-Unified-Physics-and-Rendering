#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>

namespace jhm {

struct Triangle {
    UINT vertexIndices[3];
    UINT edgeIndices[3];

    // Boundary Particle
    std::vector<UINT> innerParticlesIndices;
    UINT shortEdgeIndex = -1;
    UINT numNormalParticles;
    UINT numShortEdgeParticles;
    std::vector<UINT> lineParticles;
};
} // namespace hlab
