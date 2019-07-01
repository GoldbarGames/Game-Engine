#pragma once
#include <string>

#include "SDL.h"
#include <SDL_image.h>
#include <GL/glew.h>

#include "Vector2.h"

class Sprite
{
private:
	SDL_Texture * texture = nullptr;
	SDL_Rect textureRect;
	SDL_Rect windowRect;

	Vector2 scale = Vector2(2, 2);
	int numberFrames = 1;
	float frameWidth = 0;
	float frameHeight = 0;

public:
	void Destroy();
	const SDL_Rect* GetRect();
	void Animate(int msPerFrame);
	void Render(Vector2 position, int speed, SDL_Renderer* renderer);
	Sprite(int numFrames, SDL_Surface * image, SDL_Renderer * renderer);
	~Sprite();
};

