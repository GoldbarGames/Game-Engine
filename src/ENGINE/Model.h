#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>

#ifdef USE_ASSIMP

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

	void LoadNode(aiNode* node, const aiScene* scene);
	void LoadMesh(aiMesh* mesh, const aiScene* scene);
	void LoadMaterials(const aiScene* scene);

#endif

};

#endif