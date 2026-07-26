#include "Model.h"
#include <iostream>
#include "globals.h"

Model::Model()
{

}

Model::~Model()
{

}

#ifdef USE_ASSIMP

#ifdef MODEL_VIA_ASSIMP

void Model::LoadModel(const std::string& filename)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate
		| aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices
		| aiProcess_CalcTangentSpace);   // tangents for the scene3d normal-map path

	if (!scene)
	{
		std::cout << "Model " << filename << " failed to load: " << importer.GetErrorString() << std::endl;
		return;
	}

	LoadNode(scene->mRootNode, scene);

	LoadMaterials(scene);

}

#else  // built-in OBJ fallback (Emscripten - no assimp port)

#include <fstream>
#include <sstream>
#include <map>
#include <glm/glm.hpp>

void Model::LoadModel(const std::string& filename)
{
	LoadObjModel(filename);
}

void Model::LoadObjModel(const std::string& filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		std::cout << "Model " << filename << " failed to load: cannot open file" << std::endl;
		return;
	}

	size_t slash = filename.find_last_of("/\\");
	std::string objDir = (slash == std::string::npos) ? "" : filename.substr(0, slash + 1);

	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uvs;
	std::string mtlTexture;  // first map_Kd found (these models use one material)

	// Output: unique (position, uv) pairs, smooth normals accumulated below
	std::vector<float> vertices;              // pos3 uv2 normal3
	std::vector<unsigned int> indices;
	std::map<std::pair<int, int>, unsigned int> unique;

	auto emitVertex = [&](int vi, int ti) -> unsigned int
	{
		auto key = std::make_pair(vi, ti);
		auto it = unique.find(key);
		if (it != unique.end()) return it->second;

		unsigned int index = (unsigned int)(vertices.size() / 8);
		const glm::vec3& p = positions[vi];
		glm::vec2 uv = (ti >= 0 && ti < (int)uvs.size()) ? uvs[ti] : glm::vec2(0, 0);
		// Flip V to match the assimp path (aiProcess_FlipUVs)
		vertices.insert(vertices.end(), { p.x, p.y, p.z, uv.x, 1.0f - uv.y, 0, 0, 0 });
		unique[key] = index;
		return index;
	};

	std::string line;
	while (std::getline(file, line))
	{
		std::istringstream ss(line);
		std::string tag;
		ss >> tag;

		if (tag == "v")
		{
			glm::vec3 p;
			ss >> p.x >> p.y >> p.z;
			positions.push_back(p);
		}
		else if (tag == "vt")
		{
			glm::vec2 t;
			ss >> t.x >> t.y;
			uvs.push_back(t);
		}
		else if (tag == "f")
		{
			// Face corners as v, v/vt, v/vt/vn, or v//vn (1-based);
			// triangulate polygons as a fan
			std::vector<unsigned int> corner;
			std::string tok;
			while (ss >> tok)
			{
				int vi = 0, ti = 0;
				size_t s1 = tok.find('/');
				vi = atoi(tok.substr(0, s1).c_str());
				if (s1 != std::string::npos)
				{
					size_t s2 = tok.find('/', s1 + 1);
					std::string t = tok.substr(s1 + 1, (s2 == std::string::npos)
						? std::string::npos : s2 - s1 - 1);
					ti = atoi(t.c_str());
				}
				if (vi == 0) continue;
				corner.push_back(emitVertex(vi - 1, ti - 1));
			}
			for (size_t i = 2; i < corner.size(); i++)
			{
				indices.push_back(corner[0]);
				indices.push_back(corner[i - 1]);
				indices.push_back(corner[i]);
			}
		}
		else if (tag == "mtllib" && mtlTexture.empty())
		{
			std::string mtlName;
			ss >> mtlName;
			std::ifstream mtl(objDir + mtlName);
			std::string mtlLine;
			while (std::getline(mtl, mtlLine))
			{
				std::istringstream ms(mtlLine);
				std::string mtag;
				ms >> mtag;
				if (mtag == "map_Kd")
				{
					ms >> mtlTexture;
					break;
				}
			}
		}
	}

	if (vertices.empty() || indices.empty())
	{
		std::cout << "Model " << filename << " failed to load: no geometry" << std::endl;
		return;
	}

	// Smooth normals: accumulate face normals per unique vertex
	for (size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		unsigned int ia = indices[i], ib = indices[i + 1], ic = indices[i + 2];
		glm::vec3 a(vertices[ia * 8], vertices[ia * 8 + 1], vertices[ia * 8 + 2]);
		glm::vec3 b(vertices[ib * 8], vertices[ib * 8 + 1], vertices[ib * 8 + 2]);
		glm::vec3 c(vertices[ic * 8], vertices[ic * 8 + 1], vertices[ic * 8 + 2]);
		glm::vec3 n = glm::cross(b - a, c - a);
		for (unsigned int idx : { ia, ib, ic })
		{
			vertices[idx * 8 + 5] += n.x;
			vertices[idx * 8 + 6] += n.y;
			vertices[idx * 8 + 7] += n.z;
		}
	}
	for (size_t i = 0; i < vertices.size(); i += 8)
	{
		glm::vec3 n(vertices[i + 5], vertices[i + 6], vertices[i + 7]);
		float len = glm::length(n);
		if (len > 0.00001f) n /= len;
		vertices[i + 5] = n.x; vertices[i + 6] = n.y; vertices[i + 7] = n.z;
	}

	Mesh* newMesh = new Mesh();
	newMesh->CreateMesh(&vertices[0], &indices[0],
		(unsigned int)vertices.size(), (unsigned int)indices.size(), 8, 3, 5);
	meshList.push_back(newMesh);
	meshToTexture.push_back(0);

	// Material: map_Kd basename from textures/ (same rule as the assimp
	// path), white fallback so untextured models still render
	Texture* tex = nullptr;
	if (!mtlTexture.empty())
	{
		size_t base = mtlTexture.find_last_of("/\\");
		std::string texPath = "textures/" + (base == std::string::npos
			? mtlTexture : mtlTexture.substr(base + 1));
		tex = new Texture(texPath);
		if (!tex->LoadTexture())
		{
			delete_it(tex);
		}
	}
	if (tex == nullptr)
	{
		tex = new Texture("assets/gui/white.png");
		tex->LoadTexture();
	}
	textureList.push_back(tex);

	std::cout << "Model " << filename << " loaded via OBJ fallback: "
		<< vertices.size() / 8 << " verts, " << indices.size() / 3 << " tris"
		<< std::endl;
}

#endif  // MODEL_VIA_ASSIMP

void Model::RenderModel()
{
	for (size_t i = 0; i < meshList.size(); i++)
	{
		unsigned int materialIndex = meshToTexture[i];

		if (materialIndex < textureList.size() && textureList[materialIndex])
		{
			textureList[materialIndex]->UseTexture();
		}

		meshList[i]->RenderMesh(0);
	}
}


void Model::ClearModel()
{
	for (size_t i = 0; i < meshList.size(); i++)
	{
		if (meshList[i])
		{
			delete_it(meshList[i]);
		}
	}

	for (size_t i = 0; i < textureList.size(); i++)
	{
		if (textureList[i])
		{
			delete_it(textureList[i]);
		}
	}
}

#ifdef MODEL_VIA_ASSIMP

void Model::LoadNode(aiNode* node, const aiScene* scene)
{
	for (size_t i = 0; i < node->mNumMeshes; i++)
	{
		LoadMesh(scene->mMeshes[node->mMeshes[i]], scene);
	}

	for (size_t i = 0; i < node->mNumChildren; i++)
	{
		LoadNode(node->mChildren[i], scene);
	}
}

void Model::LoadMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	 
	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		// Insert the position
		vertices.insert(vertices.end(), { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z });

		// Insert the UV coords
		// TODO: Access all textures, not just the first one
		if (mesh->mTextureCoords[0])
		{
			vertices.insert(vertices.end(), { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y });
		}
		else
		{
			vertices.insert(vertices.end(), { 0.0f, 0.0f });
		}

		// Insert the normals
		vertices.insert(vertices.end(), { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z });

		// Insert the tangent (from aiProcess_CalcTangentSpace). Null when the
		// mesh has no UVs; store zeros so the stride stays fixed at 11.
		if (mesh->mTangents != nullptr)
			vertices.insert(vertices.end(), { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z });
		else
			vertices.insert(vertices.end(), { 0.0f, 0.0f, 0.0f });
	}


	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (size_t k = 0; k < face.mNumIndices; k++)
		{
			indices.push_back(face.mIndices[k]);
		}
	}

	Mesh* newMesh = new Mesh();
	// stride 11 = pos(3) + uv(2) + normal(3) + tangent(3); tangent at offset 8.
	newMesh->CreateMesh(&vertices[0], &indices[0], vertices.size(), indices.size(), 11, 3, 5, 8);
	meshList.push_back(newMesh);
	meshToTexture.push_back(mesh->mMaterialIndex);
}

void Model::LoadMaterials(const aiScene* scene)
{
	textureList.resize(scene->mNumMaterials);

	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* material = scene->mMaterials[i];
		textureList[i] = nullptr;

		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == aiReturn_SUCCESS)
			{
				// Use relative paths instead of absolute
				int idx = std::string(path.data).rfind('\\');
				std::string filename = std::string(path.data).substr(idx + 1);

				// Direct to our own folder
				std::string texPath = "textures/" + filename;
				textureList[i] = new Texture(texPath);

				// If we fail to load the texture...
				if (!textureList[i]->LoadTexture())
				{
					delete_it(textureList[i]);
				}
			}
		}

		if (!textureList[i])
		{
			textureList[i] = new Texture("assets/gui/white.png");
			textureList[i]->LoadTexture();
		}

	}
}

#endif  // MODEL_VIA_ASSIMP

#endif  // USE_ASSIMP