#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>

// Assimp backs model loading on desktop; Emscripten builds have no assimp
// port, so they fall back to a built-in Wavefront OBJ parser instead
// (same USE_ASSIMP game-facing API either way).
#if defined(USE_ASSIMP) && !defined(EMSCRIPTEN)
#define MODEL_VIA_ASSIMP
#endif

#ifdef MODEL_VIA_ASSIMP

// leak_check.h redefines 'new', which breaks assimp's operator new
// declarations - suspend the macro around the assimp headers
#pragma push_macro("new")
#undef new
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#pragma pop_macro("new")

#endif

#include "Mesh.h"
#include "Texture.h"

class KINJO_API Model
{
public:
	Model();
	~Model();

	std::vector<Mesh*> meshList;
	std::vector<Texture*> textureList;
	std::vector<unsigned int> meshToTexture;

#ifdef USE_ASSIMP

	void LoadModel(const std::string& filename);
	void RenderModel();
	void ClearModel();

#ifdef MODEL_VIA_ASSIMP
	void LoadNode(aiNode* node, const aiScene* scene);
	void LoadMesh(aiMesh* mesh, const aiScene* scene);
	void LoadMaterials(const aiScene* scene);
#else
	// Built-in OBJ fallback (triangulates fans, dedups v/vt pairs,
	// computes smooth normals, reads the .mtl's map_Kd from textures/)
	void LoadObjModel(const std::string& filename);
#endif

#endif

};

#endif