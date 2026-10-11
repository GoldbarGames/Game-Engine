#include "Skybox.h"
#include "Game.h"
#include "Renderer.h"
#include "Sprite.h"
#include "SpriteManager.h"
#include "Texture.h"
#include "Mesh.h"
#include "RenderState.h"
#include "SkyClouds.h"
#include "SkyBodies.h"
#include "SkyImage.h"
#include <cmath>
#include <vector>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Mesh* Skybox::meshSkySphere = nullptr;

Skybox::Skybox(Game& game, const std::string& texturePath, float radius,
	ShaderProgram* shader) : Entity(glm::vec3(0, 0, 0))
{
	etype = "skybox";
	name = "skybox";
	layer = DrawingLayer::BACK;
	drawOrder = -100;
	skyRadius = radius;
	CreateCollider(0, 0, 1, 1);

	// Build a UV sphere (same layout as the engine's shared meshes:
	// pos3, uv2, normal3)
	if (meshSkySphere == nullptr)
	{
		const int stacks = 24;
		const int slices = 48;

		std::vector<float> verts;
		std::vector<unsigned int> inds;

		for (int i = 0; i <= stacks; i++)
		{
			float phi = (float)M_PI * i / stacks;
			float y = cosf(phi);
			float r = sinf(phi);

			for (int j = 0; j <= slices; j++)
			{
				float theta = 2.0f * (float)M_PI * j / slices;
				float x = r * cosf(theta);
				float z = r * sinf(theta);

				verts.push_back(x); verts.push_back(y); verts.push_back(z);
				verts.push_back((float)j / slices); verts.push_back((float)i / stacks);
				verts.push_back(x); verts.push_back(y); verts.push_back(z);
			}
		}

		for (int i = 0; i < stacks; i++)
		{
			for (int j = 0; j < slices; j++)
			{
				unsigned int first = i * (slices + 1) + j;
				unsigned int second = first + slices + 1;
				inds.push_back(first); inds.push_back(first + 1); inds.push_back(second);
				inds.push_back(second); inds.push_back(first + 1); inds.push_back(second + 1);
			}
		}

		meshSkySphere = new Mesh();
		meshSkySphere->CreateMesh(verts.data(), inds.data(),
			(unsigned int)verts.size(), (unsigned int)inds.size(), 8, 3, 5);
	}

	// No path: a sphere for a sky the game paints itself (Scene3D::SetSkyImage)
	if (texturePath.empty())
		return;

	Texture* tex = game.spriteManager.GetImage(texturePath);
	if (tex != nullptr)
	{
		GetSprite()->SetTexture(tex);
		GetSprite()->shader = (shader != nullptr) ? shader : game.renderer.shaders[1];
		GetSprite()->mesh = meshSkySphere;
		GetSprite()->unlit = true;
		GetSprite()->frameWidth = tex->GetWidth();
		GetSprite()->frameHeight = tex->GetHeight();
	}
	else
	{
		std::cout << "ERROR: Could not load skybox texture: " << texturePath << std::endl;
	}
}

void Skybox::Update(Game& game)
{
	Entity::Update(game);

	// Keep the skybox centered on the camera so its surface always stays
	// the same distance away, no matter where the camera flies
	position = game.renderer.camera.position;
}

void Skybox::Render(const Renderer& renderer)
{
	// Keep the sphere safely inside the far plane no matter what a scene sets, so
	// it can never be clipped away (the far margin was only ~800u here).
	float r = skyRadius;
	const float maxR = renderer.camera.farPlane * 0.9f;
	if (maxR > 0.0f && r > maxR)
		r = maxR;

	// A sky the game painted itself (Scene3D::SetSkyImage), in linear HDR: it is
	// the sky while it is set, with the clouds that move over it
	if (DrawHdrSky(renderer, r))
	{
		position = renderer.camera.position;
		DrawSkyClouds(renderer, r);
		return;
	}

	Sprite* s = GetSprite();
	if (s == nullptr || s->texture == nullptr)
		return;

	// Center the sky on the camera at RENDER time. Skybox::Update also does this,
	// but it can lag (updated before the camera moves) or be skipped entirely
	// during camera glides / editor fly - letting the camera drift off the sphere
	// so its far hemisphere clips the far plane and leaves a black hole. Doing it
	// here guarantees the surface is always exactly skyRadius from the eye.
	position = renderer.camera.position;

	// The sphere mesh puts the panorama's top row (v = 0) at mesh +Y, which in
	// this -Y-up world is DOWN, so it is drawn mirrored in Y: the zenith overhead,
	// the ordinary equirectangular layout (and the one image-based lighting reads,
	// shaders/panorama.glsl). Until 2026-10-01 it was not mirrored and every sky
	// drew upside down; TrainRails and CruiseShipCleanup had flipped their
	// panoramas to compensate, and were flipped back when this was fixed.
	const glm::vec3 skyScale(r, -r, r);

	// The sun and the moon where a game has put them (SkyBodies.h): one shader
	// draws the panorama with them, in place of the two passes below.
	if (rotation == glm::vec3(0.0f) && DrawSkyWithBodies(renderer, r, s->texture, nextTexture, blendToNext,
		glm::vec3(s->color.r, s->color.g, s->color.b) / 255.0f))
	{
		DrawSkyClouds(renderer, r);
		return;
	}

	// There is no backface culling, so the sphere is visible from inside.
	// Pass 1: the base panorama (tint in colour.rgb, fully opaque).
	const Color baseColor = s->color;
	s->color.a = 255;
	s->RenderWorld(position, skyScale, rotation, renderer);

	// Pass 2: cross-fade toward the next panorama. Same geometry, so allow
	// equal-depth overwrite (LEQUAL) without writing depth, and blend by the
	// next layer's alpha. Restore state afterwards.
	if (blendToNext > 0.0f && nextTexture != nullptr)
	{
		float b = blendToNext; if (b > 1.0f) b = 1.0f;
		Texture* baseTex = s->texture;
		s->texture = nextTexture;
		s->color.a = (Uint8)(b * 255.0f);

		{
			RenderState fade = CurrentRenderState();
			fade.blend = BlendMode::Alpha;
			fade.depthCompare = CompareOp::LessEqual;
			fade.depthWrite = false;
			ScopedRenderState scope(fade);

			s->RenderWorld(position, skyScale, rotation, renderer);
		}
		s->texture = baseTex;
	}

	s->color = baseColor;

	// The clouds that move (SkyClouds.h), over whichever sky is showing.
	DrawSkyClouds(renderer, r);
}
