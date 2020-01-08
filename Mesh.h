#pragma once

#include <GL/glew.h>

class Mesh
{
public:
	Mesh();
	~Mesh();

	void CreateMesh(GLfloat* vertices, unsigned int* indices,
		unsigned int numOfVertices, unsigned int numOfIndices,
		unsigned int v = 5, unsigned int offset = 3);

	void RenderMesh();
	void ClearMesh();

private:
	GLuint VAO, VBO, IBO;
	GLsizei indexCount;
};

