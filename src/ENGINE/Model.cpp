#include "Model.h"
#include <iostream>
#include "globals.h"

Model::Model()
{

}

Model::~Model()
{

}

// ---------------------------------------------------------------------------
// Process-lifetime mesh cache: the same OBJ is loaded ONCE and its GPU meshes
// are shared by every Model that asks for it afterward (three cheap vector
// copies instead of a full assimp parse). Safe because nothing ever frees a
// Model's meshes (~Model is empty and ClearModel has no callers) - meshes
// live for the process. Without this, a tile-built scene with hundreds of
// `model` lines re-parsed the same box.obj hundreds of times and froze the
// boot for tens of seconds (found 2026-08-16, Eggwhite russet rebuild).
// NOTE: editing an .obj on disk needs a game restart to be picked up.
#include <map>
namespace
{
	std::map<std::string, Model*> gModelCache;

	Model* CacheLookup(const std::string& filename)
	{
		auto it = gModelCache.find(filename);
		return it == gModelCache.end() ? nullptr : it->second;
	}

	struct Bounds { float lo[3]; float hi[3]; };
	std::map<std::string, Bounds> gBoundsCache;
}

bool Model::LoadedBounds(const std::string& filename, float lo[3], float hi[3])
{
	auto it = gBoundsCache.find(filename);
	if (it == gBoundsCache.end())
		return false;
	for (int k = 0; k < 3; k++)
	{
		lo[k] = it->second.lo[k];
		hi[k] = it->second.hi[k];
	}
	return true;
}

namespace
{

	void CacheStore(const std::string& filename, const Model& loaded)
	{
		if (loaded.meshList.empty())
			return;   // never cache a failed load
		Model* proto = new Model();
		proto->meshList = loaded.meshList;
		proto->textureList = loaded.textureList;
		proto->meshToTexture = loaded.meshToTexture;
		gModelCache[filename] = proto;
	}
}

#ifdef USE_ASSIMP

#ifdef MODEL_VIA_ASSIMP

// ---------------------------------------------------------------------------
// On-disk mesh cache. Assimp's OBJ import (parse + JoinIdenticalVertices +
// smooth normals + tangents) is the whole cost of loading a model, and in a
// Debug build it runs at a few hundred KB/s: eight 1.6 MB character frames
// took 3.5 s, and under the debugger over a minute (found 2026-09-05, the
// Blender maid in Cruise Ship Cleanup). The first import of a file writes its
// finished vertex/index arrays to cache/<path>.kmesh; every later load reads
// that back in milliseconds and never touches assimp. The cache is keyed on
// the source file's size and modification time, so editing an .obj (or
// regenerating it) invalidates its entry on its own.
// ---------------------------------------------------------------------------
#include <chrono>
#include <cstdint>
#include <fstream>

namespace
{
	struct RawMesh
	{
		std::vector<float> vertices;       // stride 11: pos uv normal tangent
		std::vector<unsigned int> indices;
		unsigned int material = 0;
	};

	struct RawModel
	{
		std::vector<RawMesh> meshes;
		std::vector<std::string> textures;   // per material; "" = none (white)
	};

	// The model being collected by LoadNode/LoadMesh/LoadMaterials. A file
	// static rather than a member so Model.h (and the DLL's ABI) stay as they
	// were.
	RawModel* gCollect = nullptr;

	const uint32_t kCacheMagic = 0x31484D4B;   // "KMH1"

	std::string CachePath(const std::string& filename)
	{
		std::string f = filename;
		for (char& c : f)
		{
			if (c == '\\') c = '/';
			else if (c == ':') c = '_';
		}
		while (f.rfind("./", 0) == 0)
			f = f.substr(2);
		return "cache/" + f + ".kmesh";
	}

	bool SourceStamp(const std::string& filename, uint64_t& size, int64_t& mtime)
	{
		try
		{
			size = (uint64_t)fs::file_size(filename);
			mtime = (int64_t)fs::last_write_time(filename).time_since_epoch().count();
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	template <typename T>
	void Put(std::ofstream& out, const T& v)
	{
		out.write((const char*)&v, sizeof(T));
	}

	template <typename T>
	bool Get(std::ifstream& in, T& v)
	{
		in.read((char*)&v, sizeof(T));
		return in.good();
	}

	bool ReadCache(const std::string& filename, RawModel& raw)
	{
		uint64_t size = 0;
		int64_t mtime = 0;
		if (!SourceStamp(filename, size, mtime))
			return false;

		std::ifstream in(CachePath(filename), std::ios::binary);
		if (!in.is_open())
			return false;

		uint32_t magic = 0;
		uint64_t cSize = 0;
		int64_t cTime = 0;
		if (!Get(in, magic) || magic != kCacheMagic)
			return false;
		if (!Get(in, cSize) || !Get(in, cTime) || cSize != size || cTime != mtime)
			return false;   // the source changed since this was written

		uint32_t nMeshes = 0, nTex = 0;
		if (!Get(in, nMeshes) || nMeshes > 100000)
			return false;
		raw.meshes.resize(nMeshes);
		for (uint32_t i = 0; i < nMeshes; i++)
		{
			RawMesh& m = raw.meshes[i];
			uint64_t nv = 0, ni = 0;
			if (!Get(in, m.material) || !Get(in, nv) || !Get(in, ni))
				return false;
			if (nv > (1ull << 28) || ni > (1ull << 28))
				return false;
			m.vertices.resize((size_t)nv);
			m.indices.resize((size_t)ni);
			if (nv)
				in.read((char*)m.vertices.data(), (std::streamsize)(nv * sizeof(float)));
			if (ni)
				in.read((char*)m.indices.data(), (std::streamsize)(ni * sizeof(unsigned int)));
			if (!in.good())
				return false;
		}
		if (!Get(in, nTex) || nTex > 100000)
			return false;
		raw.textures.resize(nTex);
		for (uint32_t i = 0; i < nTex; i++)
		{
			uint32_t len = 0;
			if (!Get(in, len) || len > 4096)
				return false;
			raw.textures[i].resize(len);
			if (len)
				in.read(&raw.textures[i][0], len);
			if (!in.good())
				return false;
		}
		return !raw.meshes.empty();
	}

	void WriteCache(const std::string& filename, const RawModel& raw)
	{
		uint64_t size = 0;
		int64_t mtime = 0;
		if (!SourceStamp(filename, size, mtime))
			return;

		const std::string path = CachePath(filename);
		try
		{
			fs::create_directories(fs::path(path).parent_path());
		}
		catch (...)
		{
			return;
		}

		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		if (!out.is_open())
			return;

		Put(out, kCacheMagic);
		Put(out, size);
		Put(out, mtime);
		Put(out, (uint32_t)raw.meshes.size());
		for (const RawMesh& m : raw.meshes)
		{
			Put(out, m.material);
			Put(out, (uint64_t)m.vertices.size());
			Put(out, (uint64_t)m.indices.size());
			if (!m.vertices.empty())
				out.write((const char*)m.vertices.data(), (std::streamsize)(m.vertices.size() * sizeof(float)));
			if (!m.indices.empty())
				out.write((const char*)m.indices.data(), (std::streamsize)(m.indices.size() * sizeof(unsigned int)));
		}
		Put(out, (uint32_t)raw.textures.size());
		for (const std::string& t : raw.textures)
		{
			Put(out, (uint32_t)t.size());
			if (!t.empty())
				out.write(t.data(), (std::streamsize)t.size());
		}
	}
}

void Model::LoadModel(const std::string& filename)
{
	if (Model* cached = CacheLookup(filename))
	{
		meshList = cached->meshList;
		textureList = cached->textureList;
		meshToTexture = cached->meshToTexture;
		return;
	}

	const auto t0 = std::chrono::steady_clock::now();
	RawModel raw;
	const bool fromDisk = ReadCache(filename, raw);
	if (!fromDisk)
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

		gCollect = &raw;
		LoadNode(scene->mRootNode, scene);
		LoadMaterials(scene);
		gCollect = nullptr;

		WriteCache(filename, raw);
	}

	// Upload: one GPU mesh per collected mesh, one texture per material.
	Bounds b = { { 1e9f, 1e9f, 1e9f }, { -1e9f, -1e9f, -1e9f } };
	bool anyVertex = false;
	for (const RawMesh& m : raw.meshes)
	{
		if (m.vertices.empty() || m.indices.empty())
			continue;
		for (size_t i = 0; i + 2 < m.vertices.size(); i += 11)
		{
			for (int k = 0; k < 3; k++)
			{
				if (m.vertices[i + k] < b.lo[k]) b.lo[k] = m.vertices[i + k];
				if (m.vertices[i + k] > b.hi[k]) b.hi[k] = m.vertices[i + k];
			}
			anyVertex = true;
		}
		Mesh* newMesh = new Mesh();
		// stride 11 = pos(3) + uv(2) + normal(3) + tangent(3); tangent at offset 8.
		newMesh->CreateMesh((float*)m.vertices.data(), (unsigned int*)m.indices.data(),
			(unsigned int)m.vertices.size(), (unsigned int)m.indices.size(), 11, 3, 5, 8);
		meshList.push_back(newMesh);
		meshToTexture.push_back(m.material);
	}
	if (anyVertex)
		gBoundsCache[filename] = b;
	textureList.resize(raw.textures.size());
	for (size_t i = 0; i < raw.textures.size(); i++)
	{
		textureList[i] = nullptr;
		if (!raw.textures[i].empty())
		{
			// Direct to our own folder, by basename.
			const std::string& p = raw.textures[i];
			const size_t idx = p.find_last_of("/\\");
			const std::string texPath = "textures/"
				+ (idx == std::string::npos ? p : p.substr(idx + 1));
			textureList[i] = new Texture(texPath);
			if (!textureList[i]->LoadTexture())
			{
				delete_it(textureList[i]);
			}
		}
		if (!textureList[i])
		{
			textureList[i] = new Texture("assets/gui/white.png");
			textureList[i]->LoadTexture();
		}
	}

	const double ms = std::chrono::duration<double, std::milli>(
		std::chrono::steady_clock::now() - t0).count();
	if (ms > 20.0 || !fromDisk)
	{
		std::cout << "Model " << filename << (fromDisk ? " read from cache in "
			: " imported (cache written) in ") << (int)ms << " ms" << std::endl;
	}

	CacheStore(filename, *this);
}

#else  // built-in OBJ fallback (Emscripten - no assimp port)

#include <fstream>
#include <sstream>
#include <map>
#include <glm/glm.hpp>

void Model::LoadModel(const std::string& filename)
{
	if (Model* cached = CacheLookup(filename))
	{
		meshList = cached->meshList;
		textureList = cached->textureList;
		meshToTexture = cached->meshToTexture;
		return;
	}
	LoadObjModel(filename);
	CacheStore(filename, *this);
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
	// Collects into the RawModel LoadModel is building (which also feeds the
	// on-disk cache); LoadModel uploads it afterward.
	if (gCollect == nullptr)
		return;
	gCollect->meshes.push_back(RawMesh());
	RawMesh& out = gCollect->meshes.back();
	std::vector<float>& vertices = out.vertices;
	std::vector<unsigned int>& indices = out.indices;
	vertices.reserve((size_t)mesh->mNumVertices * 11);
	indices.reserve((size_t)mesh->mNumFaces * 3);

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

	out.material = mesh->mMaterialIndex;
}

void Model::LoadMaterials(const aiScene* scene)
{
	// Records each material's diffuse texture path (empty = none); LoadModel
	// turns them into Textures, from the cache or from here alike.
	if (gCollect == nullptr)
		return;
	gCollect->textures.assign(scene->mNumMaterials, std::string());

	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* material = scene->mMaterials[i];
		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == aiReturn_SUCCESS)
				gCollect->textures[i] = std::string(path.data);
		}
	}
}

#endif  // MODEL_VIA_ASSIMP

#endif  // USE_ASSIMP