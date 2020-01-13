#pragma once

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <GL/glew.h>

class ShaderProgram
{
public:
	ShaderProgram();
	~ShaderProgram();

	void CreateFromString(const char* vertexCode, const char* fragmentCode);
	void CreateFromFiles(const char* vertexFilePath, const char* fragmentFilePath);

	std::string ReadFile(const char* filePath);

	GLuint GetProjectionLocation();
	GLuint GetModelLocation();
	GLuint GetViewLocation();
	GLuint GetViewTextureLocation();
	GLuint GetOffsetTextureLocation();

	void UseShader();
	void ClearShader();

	GLuint GetID() { return programID; }

private:
	GLuint programID, uniformProjection, uniformModel, uniformView, uniformViewTexture, uniformOffsetTexture;

	void CompileShader(const char* vertexCode, const char* fragmentCode);
	void AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType);
};

