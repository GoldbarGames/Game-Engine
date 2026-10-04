#ifndef SHADER_SOURCES_H
#define SHADER_SOURCES_H
#pragma once

// Engine-internal (not exported). Shader files outside ShaderProgram's
// vertex + fragment pairs (compute shaders).

#include <string>

// "data/shaders/<name>" as the engine compiles it: the game's copy if it has
// one, else the engine's; #includes expanded; #version set for the context.
std::string LoadShaderSource(const std::string& path);

// A compute program from "data/shaders/<name>", with the engine's uniform
// blocks bound like any program's. 0 (and a logged error) on failure.
unsigned int CreateComputeProgramFromFile(const std::string& path);

#endif
