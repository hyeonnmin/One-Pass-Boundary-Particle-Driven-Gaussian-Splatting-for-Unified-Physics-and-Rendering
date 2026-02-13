#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>

namespace jhm {

struct Edge {
    UINT index0;
    UINT index1;
    float restLength;
    std::vector<UINT> edgeIndices;

    // Boundary Particle
    bool visited = false;
    int numEdgeParticles = -1;

    // Tearing
    int cutVertexIndexUp = -1;
    int cutVertexIndexDown = -1;
};

} // namespace hlab