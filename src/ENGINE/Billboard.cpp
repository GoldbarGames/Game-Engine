#include "Billboard.h"
#include "Game.h"
#include "Renderer.h"
#include "Sprite.h"
#include "SpriteManager.h"
#include "Mesh.h"
#include <cmath>
#include <iostream>

Mesh* Billboard::meshBillboardQuad = nullptr;

Billboard::Billboard(Game& game, const std::string& texturePath,
	const glm::vec3& pos, float halfExtent, ShaderProgram* shader) : Entity(pos)
{
	etype = "billboard";
	name = "billboard";
	layer = DrawingLayer::OBJECT;
	drawOrder = 2;
	size = halfExtent;
	CreateCollider(0, 0, 1, 1);

	if (meshBillboardQuad == nullptr)
	{
		// Unit quad in the X-Y plane, facing +Z; pos(3), uv(2), normal(3)
		GLfloat verts[] = {
			-1.0f, -1.0f, 0.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
			 1.0f, -1.0f, 0.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
			-1.0f,  1.0f, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
			 1.0f,  1.0f, 0.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		};
		// Both windings so the quad renders regardless of facing
		unsigned int inds[] = { 0, 1, 2, 2, 1, 3,  2, 1, 0, 3, 1, 2 };

		meshBillboardQuad = new Mesh();
		meshBillboardQuad->CreateMesh(verts, inds, 32, 12, 8, 3, 5);
	}

	Texture* tex = game.spriteManager.GetImage(texturePath);
	if (tex != nullptr)
	{
		GetSprite()->SetTexture(tex);
		GetSprite()->shader = (shader != nullptr) ? shader : game.renderer.shaders[1];
		GetSprite()->mesh = meshBillboardQuad;
		GetSprite()->unlit = true;
		GetSprite()->frameWidth = tex->GetWidth();
		GetSprite()->frameHeight = tex->GetHeight();
	}
	else
	{
		std::cout << "ERROR: Could not load billboard texture: " << texturePath << std::endl;
	}
}

void Billboard::Update(Game& game)
{
	Entity::Update(game);

	// Rotate the quad so its +Z normal points at the camera. The engine
	// applies rotations about the negative axes, which maps the quad normal
	// to (-sin ry, cos ry * sin rx, cos ry * cos rx).
	glm::vec3 dir = game.renderer.camera.position - position;
	float len = glm::length(dir);
	if (len < 0.001f)
		return;
	dir /= len;

	const float toDegrees = 180.0f / 3.14159265f;
	rotation.y = asinf(-dir.x) * toDegrees;
	rotation.x = atan2f(dir.y, dir.z) * toDegrees;
	rotation.z = 0.0f;
}

void Billboard::Render(const Renderer& renderer)
{
	if (GetSprite() == nullptr || GetSprite()->texture == nullptr)
		return;

	GetSprite()->RenderWorld(position, glm::vec3(size), rotation, renderer);
}
