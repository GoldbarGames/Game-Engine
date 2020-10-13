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
	Vector2 tileCoordinates = Vector2(0, 0);

	void Load(std::unordered_map<std::string, std::string>& map, Game& game);
	void Destroy();
	void ChangeSprite(const Vector2& frame, Texture* image, Renderer* renderer);
	void Animate();
	bool CanSpawnHere(const Vector2& spawnPosition, Game& game, bool useCamera = true);
	Tile(const Vector2& pos, const Vector2& frame, Texture* image, Renderer* renderer);
	~Tile();

	void Save(std::unordered_map<std::string, std::string>& map);

	//static Tile* __stdcall Create(const Vector2& pos) { return neww Tile(pos); };
};

#endif