#ifndef BILLBOARD_H
#define BILLBOARD_H
#pragma once

#include "Entity.h"
#include <string>

class Game;
class Mesh;
class ShaderProgram;

// A camera-facing unlit textured quad for 3D scenes: glows, flashes,
// impostors, simple particles. Uses its own quad mesh (not the engine's
// shared meshQuad) so it never takes the batched 2D path.
class KINJO_API Billboard : public Entity
{
public:
	float size = 1.0f;   // half-extent of the quad in world units

	// Unit quad with both windings, shared by all billboards
	static Mesh* meshBillboardQuad;

	// shader defaults to renderer.shaders[1] (the game's primary 3D shader)
	Billboard(Game& game, const std::string& texturePath, const glm::vec3& pos,
		float halfExtent, ShaderProgram* shader = nullptr);

	void Update(Game& game) override;
	void Render(const Renderer& renderer) override;
};

#endif
