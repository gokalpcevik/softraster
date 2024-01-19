#include "mesh.h"
#include "logger.h"

bool gfx::ImportMeshFromSceneFile(Mesh* mesh, char const* file_path, size_t mesh_index) {
    Assimp::Importer importer;

		auto* x = importer.ReadFileFromMemory(nullptr, 120212, 0);

    aiScene const* scene = importer.ReadFile(file_path, aiProcess_Triangulate);
    if (mesh_index >= scene->mNumMeshes)
        return false;
    if (scene == nullptr) {
        gfx_error("Error while importing mesh: {0}", file_path);
        return false;
    }

    aiMesh* assimp_mesh = scene->mMeshes[mesh_index];

		mesh->vertices.resize(assimp_mesh->mNumVertices);
    mesh->normals.resize(assimp_mesh->mNumVertices);
    mesh->triangles.resize(assimp_mesh->mNumFaces);

    std::memcpy(mesh->vertices.data(), assimp_mesh->mVertices, sizeof(aiVector3D) * assimp_mesh->mNumVertices);
		std::memcpy(mesh->normals.data(), assimp_mesh->mNormals, sizeof(aiVector3D) * assimp_mesh->mNumVertices);

    for (size_t face_index = 0; face_index < assimp_mesh->mNumFaces; ++face_index) {
        mesh->triangles[face_index].indices[0] = assimp_mesh->mFaces[face_index].mIndices[0];
        mesh->triangles[face_index].indices[1] = assimp_mesh->mFaces[face_index].mIndices[1];
        mesh->triangles[face_index].indices[2] = assimp_mesh->mFaces[face_index].mIndices[2];
    }

    return true;
}
