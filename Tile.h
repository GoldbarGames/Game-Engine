#ifndef TILE_H
#define TILE_H
#pragma once

#include <string>
#include <unordered_map>

#include "SDL.h"
#include <SDL_image.h>
#include <GL/glew.h>

#include "Vector2.h"
#include "Entity.h"

class Texture;
class Renderer;

class Tile : public Entity
{
public:
	void Destroy();
	const SDL_Rect* GetBounds();
	void ChangeSprite(Vector2 frame, Texture* image, Renderer* renderer);
	void Animate();
	bool CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera = true);
	Tile(Vector2 pos, Vector2 frame, Texture* image, Renderer* renderer);
	~Tile();

	void Save(std::ostringstream& level);
};

#endif