#ifndef MESH_POOL_H
#define MESH_POOL_H
#pragma once

// Engine-internal (not exported). Model meshes' vertices and indices, also
// kept in ONE shared vertex buffer + index buffer, so a single indirect
// multi-draw can draw many different meshes (GPU-driven rendering, Phase 1.5
// item 11). Model.cpp adds every mesh it builds; each Mesh keeps its own
// buffers for every other draw path.
//
// Layout as Model.cpp builds meshes: 11 floats a vertex (position, uv,
// normal, tangent), 32-bit indices. The pool's vertex array has attributes
// 0 position, 1 uv, 2 normal and 7 tangent; per-instance attributes are the
// caller's to add (location 3 is left free).

#include "RenderDevice.h"
#include <cstddef>
#include <cstdint>

class Mesh;

struct MeshPoolRange
{
	uint32_t firstIndex = 0;   // into the pool's index buffer
	uint32_t indexCount = 0;
	int32_t baseVertex = 0;    // added to each index
};

void MeshPoolAdd(const Mesh* mesh, const float* vertices, size_t floatCount,
	const unsigned int* indices, size_t indexCount);
bool MeshPoolFind(const Mesh* mesh, MeshPoolRange& out);
// Bumped whenever a mesh is added or forgotten (the GPU-driven list's cache)
uint64_t MeshPoolVersion();
void MeshPoolForget(const Mesh* mesh);   // ~Mesh (its data stays until the pool is compacted)
// Squeeze out forgotten meshes once they are a good part of the pool (a game
// that rebuilds its world: TrainRails' next route). Moves the live meshes, so
// it bumps the version; call it only where the draw list is about to be
// built (Scene3D::BuildGpuDrawList), never between building and drawing.
void MeshPoolCompact();

// The pool's vertex array, uploading whatever was added since the last call.
// No handle when the pool is empty.
VertexArrayHandle MeshPoolVertexArray();

#endif
