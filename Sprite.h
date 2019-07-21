#pragma once
#include <string>

#include "SDL.h"
#include <SDL_image.h>
#include <GL/glew.h>

#include "Vector2.h"
#include "SpriteManager.h"

class Sprite
{
private:
	SDL_Texture * texture = nullptr;
	SDL_Rect textureRect;
	
	Vector2 scale = Vector2(1, 1);
	int numberFrames = 1;
	float frameWidth = 0;
	float frameHeight = 0;
	

public:
	Vector2 pivot = Vector2(0, 0);
	std::string filename = "";
	SDL_Rect windowRect;
	void Destroy();
	const SDL_Rect* GetRect();
	void Animate(int msPerFrame);
	void Render(Vector2 position, int speed, SDL_Renderer* renderer);
	Sprite(int numFrames, SpriteManager& manager, std::string filepath, SDL_Renderer * renderer, Vector2 newPivot);
	~Sprite();
};

