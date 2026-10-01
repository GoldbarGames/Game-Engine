#ifndef UNIFORM_BLOCKS_H
#define UNIFORM_BLOCKS_H
#pragma once

// Engine-internal (not exported). Uniform block binding points, shared by the
// C++ side and the GLSL block names. GLSL 330 / WebGL2 cannot declare
// layout(binding = N), so ShaderProgram assigns every linked program's blocks
// to these points by name.
namespace UniformBlock
{
	enum Binding : unsigned int
	{
		Camera = 0,     // "Camera": view, projection (shaders/camera.glsl)
		Scene = 1,      // "Scene": lights, shadows, viewPos, time (shaders/scene.glsl)
		Material = 2,       // "Material": surface parameters (shaders/material.glsl)
		SpriteLights = 3,   // "SpriteLights": renderer point lights for lit sprites (shaders/sprite_lights.glsl)
		ShadowPass = 4,     // "ShadowPass": one shadow-map target (shaders/shadow_pass.glsl)
		Outline = 5,        // "Outline": toon outline composite settings (shaders/outline.glsl)
	};

	struct Entry { const char* name; Binding binding; };
	static const Entry kAll[] = {
		{ "Camera", Camera },
		{ "Scene", Scene },
		{ "Material", Material },
		{ "SpriteLights", SpriteLights },
		{ "ShadowPass", ShadowPass },
		{ "Outline", Outline },
	};

	// Per-draw values are not a block: each shader declares a `DrawData` struct
	// uniform named `draw` (shaders/draw.glsl, the push-constant shape). The
	// engine looks values up as "draw.<name>" first, then plain "<name>" for
	// game shaders that predate it (ShaderProgram::DrawUniformLocation).
}

#endif
