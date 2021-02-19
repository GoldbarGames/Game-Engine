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
	pointPosition, attenuationConstant, attenuationLinear, attenuationExponent, pointLightCount, spotLightCount, distanceToLight2D
};
enum class ShaderName { Default, Add, Multiply, FadeInOut, Glow, GUI, NoAlpha, SolidColor, Grid, 
	Grayscale, Sharpen, Blur, Edge, Test, Custom, Diffuse, Motion };

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
		GLuint uniformColor = 0;
		GLuint uniformAmbientIntensity = 0;
		GLuint uniformDiffuseIntensity = 0;

		GLuint uniformDirection = 0;
	} uniformDirectionalLight;

	struct
	{
		GLuint uniformColor = 0;
		GLuint uniformAmbientIntensity = 0;
		GLuint uniformDiffuseIntensity = 0;

		GLuint uniformPosition = 0;
		GLuint uniformConstant = 0;
		GLuint uniformLinear = 0;
		GLuint uniformExponent = 0;
	} uniformPointLight[MAX_POINT_LIGHTS];

	struct
	{
		GLuint uniformColor = 0;
		GLuint uniformAmbientIntensity = 0;
		GLuint uniformDiffuseIntensity = 0;

		GLuint uniformPosition = 0;
		GLuint uniformConstant = 0;
		GLuint uniformLinear = 0;
		GLuint uniformExponent = 0;

		GLuint uniformDirection = 0;
		GLuint uniformEdge = 0;

	} uniformSpotLight[MAX_SPOT_LIGHTS];

private:
	GLuint programID;
	ShaderName name;
	std::string nameString = "";
	mutable std::unordered_map<ShaderVariable, GLuint> uniformVariables;

	int pointLightCount = 0;
	int spotLightCount = 0;

	void CompileShader(const char* vertexCode, const char* fragmentCode);
	void AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType);
};

#endif