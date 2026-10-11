// The shared model mesh pool - see MeshPool.h. Backend-agnostic.
//
// The pool lives on the GPU only (since 2026-10-10). What was added since the
// last upload waits on the CPU, then goes into the buffers' spare room; when
// they run out of room they grow, copying what they hold on the GPU, and
// compacting moves the live meshes the same way. Until then the pool kept a
// CPU copy of everything, to upload the whole of it again after each addition:
// TrainRails shares its whole world here, and that copy was another 281 MB.

#include "MeshPool.h"
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace
{
	const size_t kVertexFloats = 11;   // position 3, uv 2, normal 3, tangent 3
	const size_t kVertexBytes = kVertexFloats * sizeof(float);

	// Never destroyed: meshes can outlive static destruction (at exit).
	struct Pool
	{
		// On the GPU: `usedVertices` / `usedIndices` of room for `roomVertices` / `roomIndices`.
		BufferHandle vbo, ibo;
		size_t usedVertices = 0, usedIndices = 0;
		size_t roomVertices = 0, roomIndices = 0;
		// Added since the last upload, to go after them.
		std::vector<float> pendingVertices;
		std::vector<unsigned int> pendingIndices;

		std::unordered_map<const Mesh*, MeshPoolRange> ranges;
		std::unordered_map<const Mesh*, size_t> vertexCounts;   // each mesh's vertices (for compacting)
		size_t forgottenVertices = 0;                           // of meshes forgotten since
		VertexArrayHandle vao;
		bool vaoCurrent = false;   // the vao's attributes point at these buffers
	};

	Pool& ThePool()
	{
		static Pool* pool = new Pool();
		return *pool;
	}

	uint64_t poolVersion = 0;   // MeshPoolVersion

	// New buffers with room for `vertices` and `indices`, holding what the old
	// ones held, copied on the GPU.
	void Grow(Pool& pool, size_t vertices, size_t indices)
	{
		RenderDevice& device = Device();
		BufferHandle vbo = device.CreateBuffer(std::max<size_t>(vertices, 1) * kVertexBytes, nullptr, BufferUsage::Static);
		BufferHandle ibo = device.CreateIndexBuffer(std::max<size_t>(indices, 1) * sizeof(unsigned int), nullptr);
		device.CopyBuffer(pool.vbo, 0, vbo, 0, pool.usedVertices * kVertexBytes);
		device.CopyBuffer(pool.ibo, 0, ibo, 0, pool.usedIndices * sizeof(unsigned int));
		// Draws already recorded keep the old buffers until the GPU is done with them.
		device.DestroyBuffer(pool.vbo);
		device.DestroyBuffer(pool.ibo);
		pool.vbo = vbo;
		pool.ibo = ibo;
		pool.roomVertices = vertices;
		pool.roomIndices = indices;
		pool.vaoCurrent = false;
	}

	// What was added goes up, after what is there: into buffers of just its
	// size the first time (a game's world), growing them by a quarter more
	// than they need when later additions don't fit.
	void Upload(Pool& pool)
	{
		const size_t addVertices = pool.pendingVertices.size() / kVertexFloats;
		const size_t addIndices = pool.pendingIndices.size();
		if (addVertices == 0 && addIndices == 0)
			return;
		const size_t needVertices = pool.usedVertices + addVertices;
		const size_t needIndices = pool.usedIndices + addIndices;
		if (!pool.vbo)
			Grow(pool, needVertices, needIndices);
		else if (needVertices > pool.roomVertices || needIndices > pool.roomIndices)
			Grow(pool, needVertices + needVertices / 4, needIndices + needIndices / 4);
		RenderDevice& device = Device();
		device.UpdateBuffer(pool.vbo, pool.usedVertices * kVertexBytes, addVertices * kVertexBytes,
			pool.pendingVertices.data());
		device.UpdateBuffer(pool.ibo, pool.usedIndices * sizeof(unsigned int), addIndices * sizeof(unsigned int),
			pool.pendingIndices.data());
		pool.usedVertices = needVertices;
		pool.usedIndices = needIndices;
		std::vector<float>().swap(pool.pendingVertices);
		std::vector<unsigned int>().swap(pool.pendingIndices);
		std::cout << "Mesh pool: " << pool.ranges.size() << " meshes, " << pool.usedVertices << " vertices, "
			<< pool.usedIndices / 3 << " triangles ("
			<< (pool.roomVertices * kVertexBytes + pool.roomIndices * sizeof(unsigned int)) / (1024 * 1024)
			<< " MB on the GPU)" << std::endl;
	}
}

void MeshPoolAdd(const Mesh* mesh, const float* vertices, size_t floatCount,
	const unsigned int* indices, size_t indexCount)
{
	if (mesh == nullptr || vertices == nullptr || indices == nullptr || floatCount < kVertexFloats
		|| indexCount == 0 || floatCount % kVertexFloats != 0)
		return;
	Pool& pool = ThePool();
	MeshPoolForget(mesh);   // added again: the old copy is out of date
	MeshPoolRange range;
	range.firstIndex = (uint32_t)(pool.usedIndices + pool.pendingIndices.size());
	range.indexCount = (uint32_t)indexCount;
	range.baseVertex = (int32_t)(pool.usedVertices + pool.pendingVertices.size() / kVertexFloats);
	pool.pendingVertices.insert(pool.pendingVertices.end(), vertices, vertices + floatCount);
	pool.pendingIndices.insert(pool.pendingIndices.end(), indices, indices + indexCount);
	pool.ranges[mesh] = range;
	pool.vertexCounts[mesh] = floatCount / kVertexFloats;
	poolVersion++;
}

uint64_t MeshPoolVersion()
{
	return poolVersion;
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
	if (pool.ranges.empty() || pool.ranges.erase(mesh) == 0)
		return;
	auto count = pool.vertexCounts.find(mesh);
	if (count != pool.vertexCounts.end())
	{
		pool.forgottenVertices += count->second;
		pool.vertexCounts.erase(count);
	}
	poolVersion++;
}

void MeshPoolCompact()
{
	Pool& pool = ThePool();
	const size_t held = pool.usedVertices + pool.pendingVertices.size() / kVertexFloats;
	const size_t live = held - std::min(held, pool.forgottenVertices);
	// Worth it once forgotten meshes are a third of the pool and over ~4 MB.
	if (pool.forgottenVertices * kVertexBytes < (4u << 20) || pool.forgottenVertices * 2 < live)
		return;
	size_t indices = 0;
	for (const auto& entry : pool.ranges)
		indices += entry.second.indexCount;

	// The live meshes, one after another, into buffers of just their size: the
	// uploaded ones copied on the GPU, the ones still waiting straight from the
	// CPU (so a world replacing another is never on the GPU beside it). Indices
	// are relative to a mesh's base vertex, so they copy as they are.
	RenderDevice& device = Device();
	BufferHandle vbo = device.CreateBuffer(std::max<size_t>(live, 1) * kVertexBytes, nullptr, BufferUsage::Static);
	BufferHandle ibo = device.CreateIndexBuffer(std::max<size_t>(indices, 1) * sizeof(unsigned int), nullptr);
	size_t v = 0, i = 0;
	for (auto& entry : pool.ranges)
	{
		MeshPoolRange& range = entry.second;
		auto count = pool.vertexCounts.find(entry.first);
		const size_t n = (count != pool.vertexCounts.end()) ? count->second : 0;
		const size_t indexBytes = (size_t)range.indexCount * sizeof(unsigned int);
		if ((size_t)range.baseVertex < pool.usedVertices)
		{
			device.CopyBuffer(pool.vbo, (size_t)range.baseVertex * kVertexBytes, vbo, v * kVertexBytes, n * kVertexBytes);
			device.CopyBuffer(pool.ibo, (size_t)range.firstIndex * sizeof(unsigned int), ibo, i * sizeof(unsigned int),
				indexBytes);
		}
		else
		{
			const size_t fromVertex = (size_t)range.baseVertex - pool.usedVertices;
			const size_t fromIndex = (size_t)range.firstIndex - pool.usedIndices;
			device.UpdateBuffer(vbo, v * kVertexBytes, n * kVertexBytes, pool.pendingVertices.data() + fromVertex * kVertexFloats);
			device.UpdateBuffer(ibo, i * sizeof(unsigned int), indexBytes, pool.pendingIndices.data() + fromIndex);
		}
		range.baseVertex = (int32_t)v;
		range.firstIndex = (uint32_t)i;
		v += n;
		i += range.indexCount;
	}
	device.DestroyBuffer(pool.vbo);
	device.DestroyBuffer(pool.ibo);
	pool.vbo = vbo;
	pool.ibo = ibo;
	pool.usedVertices = pool.roomVertices = v;
	pool.usedIndices = pool.roomIndices = i;
	std::vector<float>().swap(pool.pendingVertices);
	std::vector<unsigned int>().swap(pool.pendingIndices);
	pool.forgottenVertices = 0;
	pool.vaoCurrent = false;
	poolVersion++;
	std::cout << "Mesh pool: compacted to " << pool.ranges.size() << " meshes, " << v << " vertices ("
		<< (v * kVertexBytes + i * sizeof(unsigned int)) / (1024 * 1024) << " MB on the GPU)" << std::endl;
}

VertexArrayHandle MeshPoolVertexArray()
{
	Pool& pool = ThePool();
	if (pool.usedVertices == 0 && pool.pendingVertices.empty())
		return VertexArrayHandle();
	RenderDevice& device = Device();
	if (!pool.vao)
		pool.vao = device.CreateVertexArray();
	Upload(pool);
	if (!pool.vaoCurrent)
	{
		device.SetVertexAttribute(pool.vao, 0, pool.vbo, 3, kVertexBytes, 0);
		device.SetVertexAttribute(pool.vao, 1, pool.vbo, 2, kVertexBytes, 3 * sizeof(float));
		device.SetVertexAttribute(pool.vao, 2, pool.vbo, 3, kVertexBytes, 5 * sizeof(float));
		device.SetVertexAttribute(pool.vao, 7, pool.vbo, 3, kVertexBytes, 8 * sizeof(float));
		device.SetIndexBuffer(pool.vao, pool.ibo);
		pool.vaoCurrent = true;
	}
	return pool.vao;
}
