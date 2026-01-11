#ifndef MESH_H
#define MESH_H
#pragma once

#include "opengl_includes.h"
#include "leak_check.h"

#include <glm/mat4x4.hpp>

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
	void RenderMesh(unsigned int instanceAmount);
	void ClearMesh();

	GLuint GetVAO() const { return VAO; };


private:
	GLuint VAO, VBO, IBO;
	GLsizei indexCount;
	GLenum mode = GL_TRIANGLES;
};

#endif