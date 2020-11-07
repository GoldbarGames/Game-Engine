#ifndef MESH_H
#define MESH_H
#pragma once

#include <GL/glew.h>
#include "leak_check.h"
enum class MeshType { Quad, Triangle, Line, Cube };

class KINJO_API Mesh
{
public:
	Mesh();
	~Mesh();

	void CreateMesh(GLfloat* vertices, unsigned int* indices,
		unsigned int numOfVertices, unsigned int numOfIndices,
		unsigned int v = 5, unsigned int offset = 3);

	void BindMesh();
	void RenderMesh();
	void ClearMesh();

private:
	GLuint VAO, VBO, IBO;
	GLsizei indexCount;
	GLenum mode = GL_TRIANGLES;
};

#endif