#ifndef MESH_H
#define MESH_H
#pragma once

#include "opengl_includes.h"
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
	void CreateMesh(GLfloat* vertices, unsigned int* indices,
		unsigned int numOfVertices, unsigned int numOfIndices,
		unsigned int v, unsigned int uvOffset, unsigned int normalOffset,
		int tangentOffset = -1);

	void BindMesh();
	void RenderMesh(unsigned int instanceAmount);
	void ClearMesh();

	// Instanced rendering: upload per-instance model matrices to attribute
	// locations 3-6 (mat4 = 4 vec4 attributes, divisor 1 — pair with a
	// shader like instanced.vert). Once set, RenderMesh draws every instance
	// in a single glDrawElementsInstanced call. Call again to update
	// (pass dynamic = true if updating often); count 0 disables instancing.
	void SetInstances(const glm::mat4* matrices, unsigned int count, bool dynamic = false);
	unsigned int GetInstanceCount() const { return instanceCount; }

	GLuint GetVAO() const { return VAO; };


private:
	GLuint VAO, VBO, IBO;
	GLuint instanceVBO = 0;
	unsigned int instanceCount = 0;
	GLsizei indexCount;
	GLenum mode = GL_TRIANGLES;
};

#endif