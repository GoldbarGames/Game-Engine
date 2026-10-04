// The shared model mesh pool - see MeshPool.h. Backend-agnostic.

#include "MeshPool.h"
#include <unordered_map>
#include <vector>

namespace
{
	const size_t kVertexFloats = 11;   // position 3, uv 2, normal 3, tangent 3

	// Never destroyed: meshes can outlive static destruction (at exit).
	struct Pool
	{
		std::vector<float> vertices;
		std::vector<unsigned int> indices;
		std::unordered_map<const Mesh*, MeshPoolRange> ranges;
		bool dirty = false;
		VertexArrayHandle vao;
		BufferHandle vbo, ibo;
	};

	Pool& ThePool()
	{
		static Pool* pool = new Pool();
		return *pool;
	}
}

void MeshPoolAdd(const Mesh* mesh, const float* vertices, size_t floatCount,
	const unsigned int* indices, size_t indexCount)
{
	if (mesh == nullptr || vertices == nullptr || indices == nullptr || floatCount < kVertexFloats
		|| indexCount == 0 || floatCount % kVertexFloats != 0)
		return;
	Pool& pool = ThePool();
	MeshPoolRange range;
	range.firstIndex = (uint32_t)pool.indices.size();
	range.indexCount = (uint32_t)indexCount;
	range.baseVertex = (int32_t)(pool.vertices.size() / kVertexFloats);
	pool.vertices.insert(pool.vertices.end(), vertices, vertices + floatCount);
	pool.indices.insert(pool.indices.end(), indices, indices + indexCount);
	pool.ranges[mesh] = range;
	pool.dirty = true;
}

bool MeshPoolFind(const Mesh* mesh, MeshPoolRange& out)
{
	Pool& pool = ThePool();
	auto it = pool.ranges.find(mesh);
	if (it == pool.ranges.end())
		return false;
	out = it->second;
	return true;
}

void MeshPoolForget(const Mesh* mesh)
{
	Pool& pool = ThePool();
	if (!pool.ranges.empty())
		pool.ranges.erase(mesh);
}

VertexArrayHandle MeshPoolVertexArray()
{
	Pool& pool = ThePool();
	if (pool.vertices.empty())
		return VertexArrayHandle();
	RenderDevice& device = Device();
	if (!pool.vao)
		pool.vao = device.CreateVertexArray();
	if (pool.dirty)
	{
		// Rebuilt whole when meshes were added (scene loads): draws already
		// recorded keep the old buffers until the GPU is done with them.
		device.DestroyBuffer(pool.vbo);
		device.DestroyBuffer(pool.ibo);
		pool.vbo = device.CreateBuffer(pool.vertices.size() * sizeof(float), pool.vertices.data(), BufferUsage::Static);
		pool.ibo = device.CreateIndexBuffer(pool.indices.size() * sizeof(unsigned int), pool.indices.data());
		const size_t stride = kVertexFloats * sizeof(float);
		device.SetVertexAttribute(pool.vao, 0, pool.vbo, 3, stride, 0);
		device.SetVertexAttribute(pool.vao, 1, pool.vbo, 2, stride, 3 * sizeof(float));
		device.SetVertexAttribute(pool.vao, 2, pool.vbo, 3, stride, 5 * sizeof(float));
		device.SetVertexAttribute(pool.vao, 7, pool.vbo, 3, stride, 8 * sizeof(float));
		device.SetIndexBuffer(pool.vao, pool.ibo);
		pool.dirty = false;
	}
	return pool.vao;
}
