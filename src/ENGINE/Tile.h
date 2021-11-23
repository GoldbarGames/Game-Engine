#ifndef TILE_H
#define TILE_H
#pragma once

#include <string>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "opengl_includes.h"

#include "Vector2.h"
#include "Entity.h"
#include "leak_check.h"

class Texture;
class Renderer;

// This is the tile's subtype
enum class TileType { None, Grass, Wood, Metal, Ice, Water, Spike, Lava};

class KINJO_API Tile : public Entity
{
public:
	int tilesheetIndex = 0;
	glm::vec2 tileCoordinates = glm::vec2(0, 0);

	void Load(std::unordered_map<std::string, std::string>& map, Game& game);
	void Destroy();
	void ChangeSprite(const glm::vec2& frame, Texture* image, const Renderer& renderer, const int tileSize);
	void Animate();
	bool CanSpawnHere(const glm::vec3& spawnPosition, Game& game, bool useCamera = true);
	Tile(const glm::vec3& pos, const glm::vec2& frame, Texture* image, const Renderer& renderer, const int tileSize);
	~Tile();

	void Save(std::unordered_map<std::string, std::string>& map);

	//static Tile* __stdcall Create(const Vector2& pos) { return new Tile(pos); };
};

#endif