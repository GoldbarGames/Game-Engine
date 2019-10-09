#pragma once
#include <string>
#include <unordered_map>

#include "SDL.h"
#include <SDL_image.h>
#include <GL/glew.h>

#include "Vector2.h"
#include "Entity.h"

class Tile : public Entity
{
private:
	int animationFrames = 0; // how many frames to the right is the animation? (is there a better way?)

public:
	void Destroy();
	const SDL_Rect* GetBounds();
	void ChangeSprite(Vector2 frame, SDL_Texture * image, Renderer * renderer);
	void Animate();
	bool CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera = true);
	Tile(Vector2 pos, Vector2 frame, SDL_Texture * image, Renderer * renderer);
	~Tile();
};