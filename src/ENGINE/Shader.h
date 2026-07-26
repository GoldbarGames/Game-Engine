#ifndef SHADER_H
#define SHADER_H
#pragma once

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <unordered_map>

#include "opengl_includes.h"
#include "leak_check.h"
#include "globals.h"

class Renderer;

enum class ShaderVariable { model, view, projection, texFrame, texOffset, spriteColor, fadeColor, currentTime, frequency, 
	ambientIntensity, ambientColor, diffuseIntensity, lightDirection, specularIntensity, specularShine, eyePosition,
	pointPosition, attenuationConstant, attenuationLinear, attenuationExponent, pointLightCount, spotLightCount, distanceToLight2D,
	textureWidth, textureHeight
};
enum class ShaderName { Default, Add, Multiply, FadeInOut, Glow, GUI, NoAlpha, SolidColor, Grid, 
	Grayscale, Sharpen, Blur, Edge, Test, Custom, Diffuse, Motion };

class KINJO_API ShaderProgram
{
public:
	ShaderProgram(const int n, const char* vertexFilePath, const char* fragmentFilePath, bool fromString=false);

	~ShaderProgram();

	static unsigned int lastProgramID;

	// Desktop GLSL version (e.g. 460 or 330) the context actually granted; file
	// shaders have their "#version" line rewritten to this at load so a single
	// set of .vert/.frag files works whether we get a 4.6 or a 3.3 context.
	// Left at 330 and NOT applied on Emscripten (the web path keeps its own
	// GLSL ES handling untouched). Default 330.
	static int glslVersion;
	static void SetGLSLVersion(int v) { glslVersion = v; }
	// Rewrite the leading "#version ..." line of a shader source to the desktop
	// context version (no-op on Emscripten / if no #version line is present).
	static std::string ApplyVersion(const std::string& src);

	void CreateFromString(const char* vertexCode, const char* fragmentCode);
	void CreateFromFiles(const char* vertexFilePath, const char* fragmentFilePath);

	std::string ReadFile(const char* filePath);

	void UseShader() const;
	void ClearShader();

	GLuint GetID() const { return programID; }

	GLuint GetUniformVariable(ShaderVariable variable) const;

	const int& GetName() const { return name; }
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
	int name;
	std::string nameString = "";
	mutable std::unordered_map<ShaderVariable, GLuint> uniformVariables;

	int pointLightCount = 0;
	int spotLightCount = 0;

	void CompileShader(const char* vertexCode, const char* fragmentCode);
	void AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType);
};

#endif