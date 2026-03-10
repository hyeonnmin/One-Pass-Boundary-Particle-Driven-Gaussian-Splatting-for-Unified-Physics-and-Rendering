#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>
#include <string>

#include "Vertex.h"
#include "MeshData.h"

namespace jhm {

class GeometryGenerator {
  public:
    static vector<MeshData> ReadFromFile(std::string basePath,
                                         std::string filename);

    static MeshData MakeSquare();
    static MeshData MakeBox(const float scale = 1.0f);
    static MeshData MakeCylinder(const float bottomRadius,
                                 const float topRadius, float height,
                                 int numSlices);
    static MeshData MakeSphere(const float radius, const int numSlices,
                               const int numStacks);
    static MeshData MakeTriangle(const float scale);
    static MeshData MakePlane(const float scale);
    static MeshData MakeTetrahedron();
    static MeshData MakeIcosahedron();
    static MeshData SubdivideToSphere(const float radius, MeshData meshData);

    static int AddEdge(UINT index0, UINT index1, MeshData &mesh);
};
} // namespace hlab
