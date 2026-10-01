#ifndef SHADER_H
#define SHADER_H
#pragma once

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <unordered_map>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

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

	// Where a shader file actually comes from. A game's own file wins (so any
	// game can override an engine shader by keeping a copy in data/shaders);
	// a "data/shaders/..." path the game doesn't have falls back to the
	// engine's shader folder (see EngineShaderDir). Other paths pass through.
	static std::string ResolvePath(const std::string& path);

	// Location of a per-draw value in `program`: "draw.<name>" in engine
	// shaders (the DrawData struct, shaders/draw.glsl), else plain "<name>" for
	// game shaders that predate it. -1 if the program has neither.
	static int DrawUniformLocation(unsigned int program, const char* name);

	void CreateFromString(const char* vertexCode, const char* fragmentCode);
	void CreateFromFiles(const char* vertexFilePath, const char* fragmentFilePath);

	std::string ReadFile(const char* filePath);

	void UseShader() const;
	void ClearShader();

	// Set a uniform by name on this program. For game code: use these instead of
	// raw glUniform* calls. The program must be bound (UseShader) first; a name
	// the shader doesn't declare (or optimized out) is silently ignored.
	void SetInt(const char* name, int value) const;
	void SetFloat(const char* name, float value) const;
	void SetVec2(const char* name, const glm::vec2& value) const;
	void SetVec3(const char* name, const glm::vec3& value) const;
	void SetVec4(const char* name, const glm::vec4& value) const;
	void SetMat4(const char* name, const glm::mat4& value) const;
	// Same, for the engine's well-known uniforms: uses the location cached at link
	// time (see GetUniformVariable), so it's cheap enough for per-sprite hooks
	// like GUI::SetShaderVariables.
	void SetFloat(ShaderVariable variable, float value) const;
	void SetVec4(ShaderVariable variable, const glm::vec4& value) const;

	// Backend object id of the program. Engine-internal; game code sets
	// uniforms through the Set* functions above instead.
	unsigned int GetID() const { return programID; }

	unsigned int GetUniformVariable(ShaderVariable variable) const;

	const int& GetName() const { return name; }
	const std::string& GetNameString();
	void SetNameString(const std::string& s) { nameString = s; };

	struct
	{
		unsigned int uniformColor = 0;
		unsigned int uniformAmbientIntensity = 0;
		unsigned int uniformDiffuseIntensity = 0;

		unsigned int uniformDirection = 0;
	} uniformDirectionalLight;

	struct
	{
		unsigned int uniformColor = 0;
		unsigned int uniformAmbientIntensity = 0;
		unsigned int uniformDiffuseIntensity = 0;

		unsigned int uniformPosition = 0;
		unsigned int uniformConstant = 0;
		unsigned int uniformLinear = 0;
		unsigned int uniformExponent = 0;
	} uniformPointLight[MAX_POINT_LIGHTS];

	struct
	{
		unsigned int uniformColor = 0;
		unsigned int uniformAmbientIntensity = 0;
		unsigned int uniformDiffuseIntensity = 0;

		unsigned int uniformPosition = 0;
		unsigned int uniformConstant = 0;
		unsigned int uniformLinear = 0;
		unsigned int uniformExponent = 0;

		unsigned int uniformDirection = 0;
		unsigned int uniformEdge = 0;

	} uniformSpotLight[MAX_SPOT_LIGHTS];

private:
	unsigned int programID;
	int name;
	std::string nameString = "";
	mutable std::unordered_map<ShaderVariable, unsigned int> uniformVariables;

	int pointLightCount = 0;
	int spotLightCount = 0;

	void CompileShader(const char* vertexCode, const char* fragmentCode);
};

#endif