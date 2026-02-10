#pragma once

#pragma once

#include <directxtk/SimpleMath.h>
#include <string>
#include <vector>

#include "Vertex.h"
#include "VertexPBD.h"
#include "Edge.h"
#include "Triangle.h"

namespace jhm {

using std::vector;

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices; // uint32로 변경
    std::string textureFilename;

    //PBD 추가 데이터
    std::vector<VertexPBD> verticesPBD;
    std::vector<Edge> edges;
    std::vector<Triangle> triangles;

    // Volume constraint 추가 데이터
    float m_volume;

    // Tearing 추가 데이터
    std::vector<int> m_collisionVertices;
};

} // namespace hlab