#ifndef MESH_H
#define MESH_H
#pragma once

#include "opengl_includes.h"
#include "leak_check.h"
enum class MeshType { Quad, Triangle, Line, Cube, Pyramid };

class KINJO_API Mesh
{
public:
	Mesh();
	~Mesh();

	void CreateMesh(GLfloat* vertices, unsigned int* indices,
		unsigned int numOfVertices, unsigned int numOfIndices,
		unsigned int v, unsigned int uvOffset, unsigned int normalOffset);

	void BindMesh();
	void RenderMesh();
	void ClearMesh();

private:
	GLuint VAO, VBO, IBO;
	GLsizei indexCount;
	GLenum mode = GL_TRIANGLES;
};

#endif