#ifndef SKYBOX_H
#define SKYBOX_H
#pragma once

#include "Entity.h"
#include <string>

class Game;
class Mesh;
class ShaderProgram;

// A large inverted textured sphere for 3D scenes: takes any equirectangular
// panorama texture, follows the camera so it can never be left behind, and
// renders unlit. Radius must exceed the farthest scene geometry but stay
// inside the camera's far plane.
class KINJO_API Skybox : public Entity
{
public:
	float skyRadius = 3000.0f;

	// Higher-resolution sphere than the engine's shared 16x32 mesh,
	// for a smoother sky at close range (shared by all skyboxes)
	static Mesh* meshSkySphere;

	// shader defaults to renderer.shaders[1] (the game's primary 3D shader)
	Skybox(Game& game, const std::string& texturePath, float radius = 3000.0f,
		ShaderProgram* shader = nullptr);

	void Update(Game& game) override;
	void Render(const Renderer& renderer) override;
};

#endif
