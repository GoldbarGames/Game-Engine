#ifndef SHADER_H
#define SHADER_H
#pragma once

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <unordered_map>

#include <GL/glew.h>
#include "leak_check.h"
#include "globals.h"

class Renderer;

enum class ShaderVariable { model, view, projection, texFrame, texOffset, spriteColor, fadeColor, currentTime, frequency, 
	ambientIntensity, ambientColor, diffuseIntensity, lightDirection, specularIntensity, specularShine, eyePosition,
	pointPosition, attenuationConstant, attenuationLinear, attenuationExponent, pointLightCount
};
enum class ShaderName { Default, Add, Multiply, FadeInOut, Glow, GUI, NoAlpha, SolidColor, Grid, 
	Grayscale, Sharpen, Blur, Edge, Test, Custom, Diffuse };

class KINJO_API ShaderProgram
{
public:
	ShaderProgram(const ShaderName n, const char* vertexFilePath, const char* fragmentFilePath);

	~ShaderProgram();

	void CreateFromString(const char* vertexCode, const char* fragmentCode);
	void CreateFromFiles(const char* vertexFilePath, const char* fragmentFilePath);

	std::string ReadFile(const char* filePath);

	void UseShader();
	void ClearShader();

	GLuint GetID() { return programID; }

	GLuint GetUniformVariable(ShaderVariable variable) const;

	const ShaderName& GetName() { return name; }
	const std::string& GetNameString();
	void SetNameString(const std::string& s) { nameString = s; };

	struct
	{
		GLuint uniformColor;
		GLuint uniformAmbientIntensity;
		GLuint uniformDiffuseIntensity;

		GLuint uniformDirection;
	} uniformDirectionalLight;

	struct
	{
		GLuint uniformColor;
		GLuint uniformAmbientIntensity;
		GLuint uniformDiffuseIntensity;

		GLuint uniformPosition;
		GLuint uniformConstant;
		GLuint uniformLinear;
		GLuint uniformExponent;
	} uniformPointLight[MAX_POINT_LIGHTS];

private:
	GLuint programID;
	ShaderName name;
	std::string nameString = "";
	mutable std::unordered_map<ShaderVariable, GLuint> uniformVariables;

	int pointLightCount = 0;

	void CompileShader(const char* vertexCode, const char* fragmentCode);
	void AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType);
};

#endif