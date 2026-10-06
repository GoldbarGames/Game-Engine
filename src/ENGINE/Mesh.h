#ifndef MESH_H
#define MESH_H
#pragma once

#include "leak_check.h"

#include <glm/mat4x4.hpp>

// CubeTile: cube for grid tiles standing on the XY floor plane (local z =
// vertical). The +z face shows the full texture; the four side faces sample
// only the texture's bottom quarter (v 0.75..1), upright, instead of
// stretching the whole image - reads as a sculpted edge on extruded tiles.
enum class MeshType { Quad, Triangle, Line, Cube, Pyramid, Sphere, CubeTile };

class KINJO_API Mesh
{
public:
	Mesh();
	~Mesh();

	// tangentOffset >= 0 enables a tangent vertex attribute at location 7 (used
	// by the scene3d shader's vertex-tangent normal-map path). -1 = no tangent.
	// Location 7 avoids the instancing mat4 slots (locations 3-6).
	void CreateMesh(float* vertices, unsigned int* indices,
		unsigned int numOfVertices, unsigned int numOfIndices,
		unsigned int v, unsigned int uvOffset, unsigned int normalOffset,
		int tangentOffset = -1);

	// Replace the vertices of a mesh made by CreateMesh, keeping its layout and
	// its indices: the same number of floats, laid out the same way. For a shape
	// that changes every frame - a figure blended between poses (TrainRails'
	// flagman) - without building a new mesh each time. `floatCount` counts
	// floats, as CreateMesh's numOfVertices does.
	void UpdateVertices(const float* vertices, unsigned int floatCount);

	void BindMesh();
	void RenderMesh(unsigned int instanceAmount);
	void ClearMesh();

	// Instanced rendering: upload per-instance model matrices to attribute
	// locations 3-6 (mat4 = 4 vec4 attributes, divisor 1 — pair with a
	// shader like instanced.vert). Once set, RenderMesh draws every instance
	// in a single glDrawElementsInstanced call. Call again to update
	// (pass dynamic = true if updating often); count 0 disables instancing.
	void SetInstances(const glm::mat4* matrices, unsigned int count, bool dynamic = false);
	// Same, for matrices rebuilt every frame: they are streamed through the
	// engine's per-frame transient buffer instead of re-uploading the mesh's
	// own instance buffer each time. Valid for draws issued this frame.
	void SetInstancesTransient(const glm::mat4* matrices, unsigned int count);
	// Fully restore the mesh to non-instanced state: disable the instance
	// attribute arrays (3-6) on the VAO and zero the instance count. Call after
	// an instanced draw when the SAME mesh is also drawn non-instanced elsewhere
	// (e.g. the shadow depth pass) so it doesn't inherit the instance divisors.
	void ClearInstances();
	unsigned int GetInstanceCount() const { return instanceCount; }

	// Backend object id of the vertex array. Engine-internal (instanced batch
	// setup); game code should never need it.
	unsigned int GetVAO() const { return VAO; };


private:
	unsigned int VAO, VBO, IBO;
	unsigned int instanceVBO = 0;
	unsigned int instanceCount = 0;
	int indexCount;
};

#endif