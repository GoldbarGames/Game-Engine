#ifndef MESH_H
#define MESH_H
#pragma once

#include "opengl_includes.h"
#include "leak_check.h"

#include <glm/mat4x4.hpp>

enum class MeshType { Quad, Triangle, Line, Cube, Pyramid, Sphere };

class KINJO_API Mesh
{
public:
	Mesh();
	~Mesh();

	void CreateMesh(GLfloat* vertices, unsigned int* indices,
		unsigned int numOfVertices, unsigned int numOfIndices,
		unsigned int v, unsigned int uvOffset, unsigned int normalOffset);

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