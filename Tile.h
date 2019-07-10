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
	SDL_Texture * texture = nullptr;
	SDL_Rect textureRect;
	SDL_Rect windowRect;

	Vector2 scale = Vector2(1, 1);
	
	//Vector2 currentFrame = Vector2(1,1); // the selected tile among all tiles in the tilesheet
	
	Vector2 currentFrame = Vector2(0,0);
	int animationFrames = 0; // how many frames to the right is the animation? (is there a better way?)

	//std::unordered_map<Vector2, SDL_Rect>* tileMap;

public:
	void Destroy();
	const SDL_Rect* GetBounds();
	void Animate();
	void Render(SDL_Renderer* renderer, Vector2 cameraOffset);
	Tile(Vector2 frame, SDL_Surface * image, SDL_Renderer * renderer);
	Tile();
	~Tile();
};