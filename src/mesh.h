#pragma once
#include "types.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <vector>

namespace gfx {

struct Face {
    u32 indices[3];
};

struct Mesh {
    std::vector<vec3f> vertices;
		std::vector<Face> triangles;
    std::vector<vec3f> normals;
};

bool ImportMeshFromSceneFile(Mesh* mesh, char const* file_path, size_t mesh_index = 0);

} // namespace gfx
