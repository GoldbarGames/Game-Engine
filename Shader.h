#pragma once

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <unordered_map>

#include <GL/glew.h>

class Renderer;

enum class ShaderVariable { model, view, projection, texFrame, texOffset, spriteColor, fadeColor, currentTime, };

class ShaderProgram
{
public:
	ShaderProgram(const char * n, const char* vertexFilePath, const char* fragmentFilePath);

	~ShaderProgram();

	void CreateFromString(const char* vertexCode, const char* fragmentCode);
	void CreateFromFiles(const char* vertexFilePath, const char* fragmentFilePath);

	std::string ReadFile(const char* filePath);

	void UseShader();
	void ClearShader();

	GLuint GetID() { return programID; }

	GLuint GetUniformVariable(ShaderVariable variable);

	const std::string& GetName() { return name; }

private:
	GLuint programID;
	std::string name = "";
	std::unordered_map<ShaderVariable, GLuint> uniformVariables;

	void CompileShader(const char* vertexCode, const char* fragmentCode);
	void AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType);
};

